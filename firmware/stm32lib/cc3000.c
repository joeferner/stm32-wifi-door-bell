
#include "cc3000.h"
#include <stm32f10x_gpio.h>
#include <stm32f10x_exti.h>
#include <stdio.h>
#include <string.h>
#include "cc3000-host-driver/wlan.h"
#include "cc3000-host-driver/hci.h"
#include "cc3000-host-driver/netapp.h"
#include "cc3000-host-driver/cc3000_common.h"
#include "cc3000-host-driver/socket.h"
#include "cc3000-host-driver/nvmem.h"
#include "util.h"
#include "spi.h"
#include "delay.h"

#ifdef CC3000_ENABLE

#define MAXSSID           (32)
#define MAXLENGTHKEY      (32)  /* Cleared for 32 bytes by TI engineering 29/08/13 */

// The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
// for the purpose of detection of the overrun. The location of the memory where the magic number
// resides shall never be written. In case it is written - the overrun occured and either recevie function
// or send function will stuck forever.
#define CC3000_BUFFER_MAGIC_NUMBER (0xDE)

#define CC3000_SPI_STATE_POWERUP              (0)
#define CC3000_SPI_STATE_INITIALIZED          (1)
#define CC3000_SPI_STATE_IDLE                 (2)
#define CC3000_SPI_STATE_WRITE_IRQ            (3)
#define CC3000_SPI_STATE_WRITE_FIRST_PORTION  (4)
#define CC3000_SPI_STATE_WRITE_EOT            (5)
#define CC3000_SPI_STATE_READ_IRQ             (6)
#define CC3000_SPI_STATE_READ_FIRST_PORTION   (7)
#define CC3000_SPI_STATE_READ_EOT             (8)

#define CC3000_HEADERS_SIZE_EVNT              (SPI_HEADER_SIZE + 5)
#define CC3000_SPI_HEADER_SIZE                (5)

#define CC3000_READ                            (3)
#define CC3000_WRITE                           (1)
#define CC3000_HI(value)                       (((value) & 0xFF00) >> 8)
#define CC3000_LO(value)                       ((value) & 0x00FF)

#define CC3000_SUCCESS 0

typedef struct {
  BOOL isSmartConfigFinished: 1;
  BOOL isConnected: 1;
  BOOL hasDHCP: 1;
  BOOL okToShutdown: 1;
} _CC3000Status;

typedef struct {
  _cc3000_spiHandleRx spiRxHandler;
  uint16_t txPacketLength;
  uint16_t rxPacketLength;
  uint16_t spiState;
  uint8_t* txPacket;
  uint8_t* rxPacket;
} _CC3000SpiInformation;

_CC3000Status _cc3000_status;
_CC3000SpiInformation _cc3000_spiInformation;
BOOL _cc3000_closedSockets[MAX_SOCKETS];
uint8_t _cc3000_pingReportNum;
netapp_pingreport_args_t _cc3000_pingReport;
BOOL _cc3000_irqEnabled;
BOOL _cc3000_inIrq;
uint8_t spi_buffer[CC3000_RX_BUFFER_SIZE];

// used by wlan.c
uint8_t wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

void _cc3000_cs_assert();
void _cc3000_cs_deassert();
void _cc3000_en_assert();
void _cc3000_en_deassert();
void _cc3000_asyncCallback(long lEventType, char* data, unsigned char length);
char* _cc3000_sendFWPatches(unsigned long* length);
char* _cc3000_sendDriverPatches(unsigned long* length);
char* _cc3000_sendBootLoaderPatches(unsigned long* length);
long _cc3000_readWlanInterruptPin();
void _cc3000_wlanInterruptEnable();
void _cc3000_wlanInterruptDisable();
void _cc3000_writeWlanPin(unsigned char val);
uint8_t _cc3000_spiTransfer(uint8_t d);
void _cc3000_spiReadHeader();
void _cc3000_spiReadDataSynchronous(uint8_t* data, uint16_t size);
void _cc3000_spiWriteDataSynchronous(uint8_t* data, uint16_t size);
void _cc3000_ssiContReadOperation();
BOOL _cc3000_spiReadDataCont();
void _cc3000_spiTriggerRxProcessing();
void _cc3000_spiFirstWrite(uint8_t* buffer, uint16_t length);
BOOL _cc3000_scanSSIDs(uint32_t time);
BOOL _cc3000_connectOpen(const char* ssid);
BOOL _cc3000_connectSecure(const char* ssid, const char* key, int32_t secMode);
BOOL _cc3000_getFirmwareVersion(uint8_t* major, uint8_t* minor);

void cc3000_setupGpio() {
  GPIO_InitTypeDef gpioInitStructure;

  printf("BEGIN CC3000 Setup GPIO\n");

  if (CC3000_RCC1) {
    RCC_APB1PeriphClockCmd(CC3000_RCC1, ENABLE);
  }
  if (CC3000_RCC2) {
    RCC_APB2PeriphClockCmd(CC3000_RCC2, ENABLE);
  }

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  gpioInitStructure.GPIO_Pin = CC3000_CS_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CC3000_CS, &gpioInitStructure);

  gpioInitStructure.GPIO_Pin = CC3000_IRQ_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CC3000_IRQ, &gpioInitStructure);

  gpioInitStructure.GPIO_Pin = CC3000_EN_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CC3000_EN, &gpioInitStructure);

  _cc3000_cs_deassert();
  _cc3000_en_deassert();

  printf("END CC3000 Setup GPIO\n");
}

BOOL cc3000_setup(uint8_t patchReq, BOOL useSmartConfigData, const char* deviceName) {
  printf("BEGIN CC3000 Setup (patchReq: %d, useSmartConfigData: %s, deviceName: %s)\n", patchReq, useSmartConfigData ? "TRUE" : "FALSE", deviceName);

  _cc3000_pingReportNum = 0;
  _cc3000_irqEnabled = FALSE;
  memset(&_cc3000_status, 0, sizeof(_cc3000_status));
  memset(&_cc3000_pingReport, 0, sizeof(_cc3000_pingReport));
  for (int i = 0; i < MAX_SOCKETS; i++) {
    _cc3000_closedSockets[i] = FALSE;
  }
  _cc3000_inIrq = FALSE;

  wlan_init(
    _cc3000_asyncCallback,
    _cc3000_sendFWPatches,
    _cc3000_sendDriverPatches,
    _cc3000_sendBootLoaderPatches,
    _cc3000_readWlanInterruptPin,
    _cc3000_wlanInterruptEnable,
    _cc3000_wlanInterruptDisable,
    _cc3000_writeWlanPin
  );

  wlan_start(patchReq);

  uint8_t firmwareMajor, firmwareMinor;
  if (_cc3000_getFirmwareVersion(&firmwareMajor, &firmwareMinor)) {
    printf("CC3000 firmware: %d.%d\n", firmwareMajor, firmwareMinor);
  } else {
    printf("failed to get firmware\n");
    return FALSE;
  }

  // Check if we should erase previous stored connection details
  // (most likely written with data from the SmartConfig app)
  if (!useSmartConfigData) {
    // Manual connection only (no auto, profiles, etc.)
    wlan_ioctl_set_connection_policy(0, 0, 0);
    // Delete previous profiles from memory
    wlan_ioctl_del_profile(255);
  } else {
    // Auto Connect - the C3000 device tries to connect to any AP it detects during scanning:
    // wlan_ioctl_set_connection_policy(1, 0, 0)

    // Fast Connect - the CC3000 device tries to reconnect to the last AP connected to:
    // wlan_ioctl_set_connection_policy(0, 1, 0)

    // Use Profiles - the CC3000 device tries to connect to an AP from profiles:
    wlan_ioctl_set_connection_policy(0, 0, 1);
  }

  if (wlan_set_event_mask(HCI_EVNT_WLAN_UNSOL_INIT        |
                          //HCI_EVNT_WLAN_ASYNC_PING_REPORT |// we want ping reports
                          //HCI_EVNT_BSD_TCP_CLOSE_WAIT |
                          //HCI_EVNT_WLAN_TX_COMPLETE |
                          HCI_EVNT_WLAN_KEEPALIVE) != CC3000_SUCCESS) {
    printf("WLAN Set Event Mask FAIL\n");
    return FALSE;
  }

  // Wait for re-connection if we're using SmartConfig data
  if (useSmartConfigData) {
    // Wait for a connection
    uint32_t timeout = 0;
    while (!_cc3000_status.isConnected) {
      _cc3000_irqPoll();
      if (timeout > WLAN_CONNECT_TIMEOUT) {
        return FALSE;
      }
      timeout += 10;
      delay_ms(10);
    }

    delay_ms(1000);
    if (_cc3000_status.hasDHCP) {
      mdnsAdvertiser(1, (char*) deviceName, strlen(deviceName));
    }
  }

  printf("END CC3000 Setup\n");
  return TRUE;
}

void _cc3000_cs_assert() {
  GPIO_ResetBits(CC3000_CS, CC3000_CS_PIN);
}

void _cc3000_cs_deassert() {
  GPIO_SetBits(CC3000_CS, CC3000_CS_PIN);
}

void _cc3000_en_assert() {
  GPIO_SetBits(CC3000_EN, CC3000_EN_PIN);
}

void _cc3000_en_deassert() {
  GPIO_ResetBits(CC3000_EN, CC3000_EN_PIN);
}

extern void SpiReceiveHandler(void* pvBuffer);

BOOL _cc3000_getFirmwareVersion(uint8_t* major, uint8_t* minor) {
  uint8_t fwpReturn[2];

  if (nvmem_read_sp_version(fwpReturn) != CC3000_SUCCESS) {
    printf("Unable to read the firmware version\n");
    return FALSE;
  }

  *major = fwpReturn[0];
  *minor = fwpReturn[1];

  return TRUE;
}

BOOL cc3000_connectToAP(const char* ssid, const char* key, uint8_t secmode) {
  int16_t timer;

  do {
    _cc3000_irqPoll();

    /* MEME: not sure why this is absolutely required but the cc3k freaks
       if you dont. maybe bootup delay? */
    // Setup a 4 second SSID scan
    _cc3000_scanSSIDs(4000);
    // Wait for results
    delay_ms(4500);
    _cc3000_scanSSIDs(0);

    /* Attempt to connect to an access point */
    printf("Connecting to %s\n", ssid);
    if ((secmode == 0) || (strlen(key) == 0)) {
      /* Connect to an unsecured network */
      if (!_cc3000_connectOpen(ssid)) {
        printf("Failed to connect\n");
        continue;
      }
    } else {
      /* Connect to a secure network using WPA2, etc */
      if (!_cc3000_connectSecure(ssid, key, secmode)) {
        printf("Failed to connect\n");
        continue;
      }
    }

    timer = WLAN_CONNECT_TIMEOUT;

    /* Wait around a bit for the async connected signal to arrive or timeout */
    printf("Waiting to connect...\n");
    while ((timer > 0) && !_cc3000_status.isConnected) {
      _cc3000_irqPoll();
      delay_ms(10);
      timer -= 10;
    }
    if (timer <= 0) {
      printf("Timed out!\n");
    }
  } while (!_cc3000_status.isConnected);

  return TRUE;
}

BOOL _cc3000_scanSSIDs(uint32_t time) {
  const unsigned long intervalTime[16] = { 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000 };

  // We can abort a scan with a time of 0
  if (time) {
    printf("Started AP/SSID scan\n");
  }

  // Set  SSID Scan params to includes channels above 11
  if (wlan_ioctl_set_scan_params(time, 20, 100, 5, 0x1FFF, -120, 0, 300, (unsigned long*) &intervalTime) != CC3000_SUCCESS) {
    printf("Failed setting params for SSID scan\n");
    return FALSE;
  }

  return TRUE;
}

BOOL _cc3000_connectOpen(const char* ssid) {
  if (wlan_ioctl_set_connection_policy(0, 0, 0) != CC3000_SUCCESS) {
    printf("Failed to set connection policy\n");
    return FALSE;
  }
  delay_ms(500);
  if (wlan_connect(WLAN_SEC_UNSEC, (char*)ssid, strlen(ssid), 0 , NULL, 0) != CC3000_SUCCESS) {
    printf("SSID connection failed\n");
    return FALSE;
  }
  return TRUE;
}

BOOL _cc3000_connectSecure(const char* ssid, const char* key, int32_t secMode) {
  if ((secMode < 0) || (secMode > 3)) {
    printf("Security mode must be between 0 and 3\n");
    return FALSE;
  }

  if (strlen(ssid) > MAXSSID) {
    printf("SSID length must be < %d", MAXSSID);
    return FALSE;
  }

  if (strlen(key) > MAXLENGTHKEY) {
    printf("Key length must be < %d", MAXLENGTHKEY);
    return FALSE;
  }

  if (wlan_ioctl_set_connection_policy(0, 0, 0) != CC3000_SUCCESS) {
    printf("Failed setting the connection policy\n");
    return FALSE;
  };
  delay_ms(500);
  if (wlan_connect(secMode, (char*)ssid, strlen(ssid), NULL, (unsigned char*)key, strlen(key)) != CC3000_SUCCESS) {
    printf("SSID connection failed\n");
    return FALSE;
  }

  /* Wait for 'HCI_EVNT_WLAN_UNSOL_CONNECT' in CC3000_UsynchCallback */
  return TRUE;
}

void _cc3000_asyncCallback(long lEventType, char* data, unsigned char length) {
  if (lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE) {
    _cc3000_status.isSmartConfigFinished = TRUE;
  } else if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT) {
    _cc3000_status.isConnected = TRUE;
  } else if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT) {
    _cc3000_status.isConnected = FALSE;
    _cc3000_status.hasDHCP = FALSE;
  } else if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP) {
    _cc3000_status.hasDHCP = TRUE;
  } else if (lEventType == HCI_EVENT_CC3000_CAN_SHUT_DOWN) {
    _cc3000_status.okToShutdown = TRUE;
  } else if (lEventType == HCI_EVNT_WLAN_ASYNC_PING_REPORT) {
    _cc3000_pingReportNum++;
    memcpy(&_cc3000_pingReport, data, length);
  } else if (lEventType == HCI_EVNT_BSD_TCP_CLOSE_WAIT) {
    uint8_t socketnum;
    socketnum = data[0];
    if (socketnum < MAX_SOCKETS) {
      _cc3000_closedSockets[socketnum] = TRUE;
    }
  }
}

char* _cc3000_sendFWPatches(unsigned long* length) {
  *length = 0;
  return NULL;
}

char* _cc3000_sendDriverPatches(unsigned long* length) {
  *length = 0;
  return NULL;
}

char* _cc3000_sendBootLoaderPatches(unsigned long* length) {
  *length = 0;
  return NULL;
}

long _cc3000_readWlanInterruptPin() {
  return GPIO_ReadInputDataBit(CC3000_IRQ, CC3000_IRQ_PIN) == Bit_SET;
}

void _cc3000_wlanInterruptEnable() {
  EXTI_InitTypeDef extiInitStructure;
  NVIC_InitTypeDef nvicInitStructure;

  GPIO_EXTILineConfig(CC3000_IRQ_EXTI_PORT, CC3000_IRQ_EXTI_PIN);

  EXTI_StructInit(&extiInitStructure);
  extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  extiInitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  extiInitStructure.EXTI_LineCmd = ENABLE;
  extiInitStructure.EXTI_Line = CC3000_IRQ_EXTI_LINE;
  EXTI_Init(&extiInitStructure);

  nvicInitStructure.NVIC_IRQChannel = CC3000_IRQ_EXTI_CH;
  nvicInitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStructure);

  _cc3000_irqEnabled = TRUE;
}

void _cc3000_wlanInterruptDisable() {
  EXTI_InitTypeDef extiInitStructure;

  EXTI_StructInit(&extiInitStructure);
  extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  extiInitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  extiInitStructure.EXTI_LineCmd = DISABLE;
  extiInitStructure.EXTI_Line = CC3000_IRQ_EXTI_LINE;
  EXTI_Init(&extiInitStructure);

  _cc3000_irqEnabled = FALSE;
}

void _cc3000_writeWlanPin(unsigned char val) {
  if (val) {
    GPIO_SetBits(CC3000_EN, CC3000_EN_PIN);
  } else {
    GPIO_ResetBits(CC3000_EN, CC3000_EN_PIN);
  }
}

uint8_t _cc3000_spiTransfer(uint8_t d) {
  return spi_transfer(CC3000_SPI, d);
}

void _cc3000_spiReadHeader() {
  _cc3000_spiReadDataSynchronous(_cc3000_spiInformation.rxPacket, CC3000_HEADERS_SIZE_EVNT);
}

void _cc3000_spiReadDataSynchronous(uint8_t* data, uint16_t size) {
  unsigned short i = 0;

  for (i = 0; i < size; i ++) {
    data[i] = _cc3000_spiTransfer(0x03);
  }
}

void _cc3000_ssiContReadOperation() {
  /* The header was read - continue with  the payload read */
  if (_cc3000_spiReadDataCont()) {
    /* All the data was read - finalize handling by switching to teh task
     *  and calling from task Event Handler */
    _cc3000_spiTriggerRxProcessing();
  }
}

void _cc3000_spiTriggerRxProcessing() {
  /* Trigger Rx processing */
  _cc3000_wlanInterruptDisable();
  _cc3000_cs_deassert();

  /* The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
   * for the purpose of detection of the overrun. If the magic number is overriten - buffer overrun
   * occurred - and we will stuck here forever! */
  if (_cc3000_spiInformation.rxPacket[CC3000_RX_BUFFER_SIZE - 1] != CC3000_BUFFER_MAGIC_NUMBER) {
    /* You've got problems if you're here! */
    printf("CC3000: ERROR - magic number missing!\n");
    while (1);
  }

  _cc3000_spiInformation.spiState = CC3000_SPI_STATE_IDLE;
  _cc3000_spiInformation.spiRxHandler(_cc3000_spiInformation.rxPacket + SPI_HEADER_SIZE);
}

BOOL _cc3000_spiReadDataCont() {
  long data_to_recv;
  uint8_t* evnt_buff, type;

  /* Determine what type of packet we have */
  evnt_buff = _cc3000_spiInformation.rxPacket;
  data_to_recv = 0;
  STREAM_TO_UINT8((uint8_t*)(evnt_buff + SPI_HEADER_SIZE), HCI_PACKET_TYPE_OFFSET, type);

  switch (type) {
    case HCI_TYPE_DATA: {
      /* We need to read the rest of data.. */
      STREAM_TO_UINT16((char*)(evnt_buff + SPI_HEADER_SIZE), HCI_DATA_LENGTH_OFFSET, data_to_recv);
      if (!((CC3000_HEADERS_SIZE_EVNT + data_to_recv) & 1)) {
        data_to_recv++;
      }

      if (data_to_recv) {
        _cc3000_spiReadDataSynchronous(evnt_buff + CC3000_HEADERS_SIZE_EVNT, data_to_recv);
      }
      break;
    }
    case HCI_TYPE_EVNT: {
      /* Calculate the rest length of the data */
      STREAM_TO_UINT8((char*)(evnt_buff + SPI_HEADER_SIZE), HCI_EVENT_LENGTH_OFFSET, data_to_recv);
      data_to_recv -= 1;

      /* Add padding byte if needed */
      if ((CC3000_HEADERS_SIZE_EVNT + data_to_recv) & 1) {
        data_to_recv++;
      }

      if (data_to_recv) {
        _cc3000_spiReadDataSynchronous(evnt_buff + CC3000_HEADERS_SIZE_EVNT, data_to_recv);
      }

      _cc3000_spiInformation.spiState = CC3000_SPI_STATE_READ_EOT;
      break;
    }
  }

  return TRUE;
}

void _cc3000_spiWriteDataSynchronous(uint8_t* data, uint16_t size) {
  uint16_t loc;
  for (loc = 0; loc < size; loc ++) {
    _cc3000_spiTransfer(data[loc]);
  }
}

void _cc3000_spiOpen(_cc3000_spiHandleRx rxHandler) {
  _cc3000_spiInformation.spiState = CC3000_SPI_STATE_POWERUP;

  memset(spi_buffer, 0, sizeof(spi_buffer));
  memset(wlan_tx_buffer, 0, sizeof(wlan_tx_buffer));

  _cc3000_spiInformation.spiRxHandler = rxHandler;
  _cc3000_spiInformation.txPacketLength = 0;
  _cc3000_spiInformation.txPacket = NULL;
  _cc3000_spiInformation.rxPacket = spi_buffer;
  _cc3000_spiInformation.rxPacketLength = 0;

  spi_buffer[CC3000_RX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;
  wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;

  /* Enable interrupt on the GPIO pin of WLAN IRQ */
  tSLInformation.WlanInterruptEnable();
}

void _cc3000_spiResume() {
  _cc3000_wlanInterruptEnable();
}

void _cc3000_spiFirstWrite(uint8_t* buffer, uint16_t length) {
  /* Workaround for the first transaction */
  _cc3000_cs_assert();

  /* delay (stay low) for ~50us */
  delay_ms(1);

  /* SPI writes first 4 bytes of data */
  _cc3000_spiWriteDataSynchronous(buffer, 4);

  delay_ms(1);

  _cc3000_spiWriteDataSynchronous(buffer + 4, length - 4);

  /* From this point on - operate in a regular manner */
  _cc3000_spiInformation.spiState = CC3000_SPI_STATE_IDLE;

  _cc3000_cs_deassert();
}

void _cc3000_spiWrite(uint8_t* buffer, uint16_t length) {
  uint8_t ucPad = 0;

  /* Figure out the total length of the packet in order to figure out if there is padding or not */
  if (!(length & 0x0001)) {
    ucPad++;
  }

  buffer[0] = CC3000_WRITE;
  buffer[1] = CC3000_HI(length + ucPad);
  buffer[2] = CC3000_LO(length + ucPad);
  buffer[3] = 0;
  buffer[4] = 0;

  length += (SPI_HEADER_SIZE + ucPad);

  /* The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
   * for the purpose of overrun detection. If the magic number is overwritten - buffer overrun
   * occurred - and we will be stuck here forever! */
  if (wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] != CC3000_BUFFER_MAGIC_NUMBER) {
    printf("CC3000: Error - No magic number found in SpiWrite\n");
    while (1);
  }

  if (_cc3000_spiInformation.spiState == CC3000_SPI_STATE_POWERUP) {
    while (_cc3000_spiInformation.spiState != CC3000_SPI_STATE_INITIALIZED);
  }

  if (_cc3000_spiInformation.spiState == CC3000_SPI_STATE_INITIALIZED) {
    /* This is time for first TX/RX transactions over SPI: the IRQ is down - so need to send read buffer size command */
    _cc3000_spiFirstWrite(buffer, length);
  } else {
    /* We need to prevent here race that can occur in case two back to back packets are sent to the
     * device, so the state will move to IDLE and once again to not IDLE due to IRQ */
    tSLInformation.WlanInterruptDisable();

    while (_cc3000_spiInformation.spiState != CC3000_SPI_STATE_IDLE);

    _cc3000_spiInformation.spiState = CC3000_SPI_STATE_WRITE_IRQ;
    _cc3000_spiInformation.txPacket = buffer;
    _cc3000_spiInformation.txPacketLength = length;

    /* Assert the CS line and wait till SSI IRQ line is active and then initialize write operation */
    _cc3000_cs_assert();

    /* Re-enable IRQ - if it was not disabled - this is not a problem... */
    tSLInformation.WlanInterruptEnable();

    /* Check for a missing interrupt between the CS assertion and enabling back the interrupts */
    if (tSLInformation.ReadWlanInterruptPin() == 0) {
      _cc3000_spiWriteDataSynchronous(_cc3000_spiInformation.txPacket, _cc3000_spiInformation.txPacketLength);

      _cc3000_spiInformation.spiState = CC3000_SPI_STATE_IDLE;

      _cc3000_cs_deassert();
#ifdef SPI_HAS_TRANSACTION
      WlanInterruptEnable();
#endif
    }
  }

  /* Due to the fact that we are currently implementing a blocking situation
   * here we will wait till end of transaction */
  while (CC3000_SPI_STATE_IDLE != _cc3000_spiInformation.spiState);
}

void _cc3000_irqPoll() {
  if ((_cc3000_readWlanInterruptPin() == Bit_RESET) && !_cc3000_inIrq && _cc3000_irqEnabled) {
    _cc3000_irq();
  }
}

void _cc3000_irq() {
  _cc3000_inIrq = TRUE;

  if (_cc3000_spiInformation.spiState == CC3000_SPI_STATE_POWERUP) {
    /* IRQ line was low ... perform a callback on the HCI Layer */
    _cc3000_spiInformation.spiState = CC3000_SPI_STATE_INITIALIZED;
  } else if (_cc3000_spiInformation.spiState == CC3000_SPI_STATE_IDLE) {
    _cc3000_spiInformation.spiState = CC3000_SPI_STATE_READ_IRQ;

    /* IRQ line goes down - start reception */
    _cc3000_cs_assert();

    // Wait for TX/RX Compete which will come as DMA interrupt
    _cc3000_spiReadHeader();
    _cc3000_spiInformation.spiState = CC3000_SPI_STATE_READ_EOT;
    _cc3000_ssiContReadOperation();
  } else if (_cc3000_spiInformation.spiState == CC3000_SPI_STATE_WRITE_IRQ) {
    _cc3000_spiWriteDataSynchronous(_cc3000_spiInformation.txPacket, _cc3000_spiInformation.txPacketLength);
    _cc3000_spiInformation.spiState = CC3000_SPI_STATE_IDLE;
    _cc3000_cs_deassert();
  }

  _cc3000_inIrq = FALSE;
}

#endif

