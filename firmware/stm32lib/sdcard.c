
#include "sdcard.h"
#include "time.h"
#include <stdio.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include "sdcard-info.h"

#ifdef SDCARD_ENABLE

#define SDCARD_BLOCK_SIZE 512

/** Standard capacity V1 SD card */
uint8_t const SD_CARD_TYPE_SD1 = 1;
/** Standard capacity V2 SD card */
uint8_t const SD_CARD_TYPE_SD2 = 2;
/** High Capacity SD card */
uint8_t const SD_CARD_TYPE_SDHC = 3;

/** init timeout ms */
uint16_t const SD_INIT_TIMEOUT = 2000;
/** erase timeout ms */
uint16_t const SD_ERASE_TIMEOUT = 10000;
/** read timeout ms */
uint16_t const SD_READ_TIMEOUT = 300;
/** write time out ms */
uint16_t const SD_WRITE_TIMEOUT = 600;

uint8_t _sdcard_type;

void _sdcard_cs_deassert();
void _sdcard_cs_assert();
uint8_t _sdcard_spi_transfer(uint8_t d);
uint8_t _sdcard_command(uint8_t cmd, uint32_t arg);
uint8_t _sdcard_acommand(uint8_t acmd, uint32_t arg);
uint8_t _sdcard_waitStartBlock();
uint8_t _sdcard_waitNotBusy(uint16_t timeoutMillis);

void sdcard_setupGpio() {
  GPIO_InitTypeDef gpioInitStructure;

  printf("BEGIN SDCard Setup GPIO\n");

  RCC_APB1PeriphClockCmd(SDCARD_RCC1, ENABLE);
  RCC_APB2PeriphClockCmd(SDCARD_RCC2, ENABLE);

  gpioInitStructure.GPIO_Pin = SDCARD_CS_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SDCARD_CS, &gpioInitStructure);

  _sdcard_cs_deassert();

  printf("END SDCard Setup GPIO\n");
}

BOOL sdcard_setup() {
  // 16-bit init start time allows over a minute
  uint16_t t0 = (uint16_t)time_ms();
  uint32_t arg;
  uint8_t status;

  printf("BEGIN SDCard Setup\n");

  // must supply min of 74 clock cycles with CS high.
  for (uint8_t i = 0; i < 10; i++) {
    _sdcard_spi_transfer(0XFF);
  }

  _sdcard_cs_assert();

  // command to go idle in SPI mode
  while ((status = _sdcard_command(CMD0, 0)) != R1_IDLE_STATE) {
    if (((uint16_t)time_ms() - t0) > SD_INIT_TIMEOUT) {
      printf("SD_CARD_ERROR_CMD0\n");
      goto fail;
    }
  }

  // check SD version
  if ((_sdcard_command(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
    _sdcard_type = SD_CARD_TYPE_SD1;
  } else {
    // only need last byte of r7 response
    for (uint8_t i = 0; i < 4; i++) {
      status = _sdcard_spi_transfer(0xff);
    }
    if (status != 0XAA) {
      printf("SD_CARD_ERROR_CMD8\n");
      goto fail;
    }
    _sdcard_type = SD_CARD_TYPE_SD2;
  }

  // initialize card and send host supports SDHC if SD2
  arg = _sdcard_type == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

  while ((status = _sdcard_acommand(ACMD41, arg)) != R1_READY_STATE) {
    // check for timeout
    if (((uint16_t)time_ms() - t0) > SD_INIT_TIMEOUT) {
      printf("SD_CARD_ERROR_ACMD41\n");
      goto fail;
    }
  }

  // if SD2 read OCR register to check for SDHC card
  if (_sdcard_type == SD_CARD_TYPE_SD2) {
    if (_sdcard_command(CMD58, 0)) {
      printf("SD_CARD_ERROR_CMD58\n");
      goto fail;
    }
    if ((_sdcard_spi_transfer(0xff) & 0XC0) == 0XC0) {
      _sdcard_type = SD_CARD_TYPE_SDHC;
    }
    // discard rest of ocr - contains allowed voltage range
    for (uint8_t i = 0; i < 3; i++) {
      _sdcard_spi_transfer(0xff);
    }
  }

  _sdcard_cs_deassert();
  printf("END SDCard Setup (type: %u)\n", _sdcard_type);

  return TRUE;

fail:
  _sdcard_cs_deassert();
  printf("END SDCard Setup (Fail)\n");
  return FALSE;
}

void _sdcard_cs_deassert() {
  GPIO_SetBits(SDCARD_CS, SDCARD_CS_PIN);
}

void _sdcard_cs_assert() {
  GPIO_ResetBits(SDCARD_CS, SDCARD_CS_PIN);
}

BOOL sdcard_readBlock(uint32_t block, uint8_t* data) {
  uint16_t n;

  // use address if not SDHC card
  if (_sdcard_type != SD_CARD_TYPE_SDHC) {
    block <<= 9;
  }

  if (_sdcard_command(CMD17, block)) {
    printf("SD_CARD_ERROR_CMD17\n");
    goto fail;
  }

  if (!_sdcard_waitStartBlock()) {
    goto fail;
  }

  for (n = 0; n < SDCARD_BLOCK_SIZE; n++) {
    data[n] = _sdcard_spi_transfer(0xff);
  }

  _sdcard_cs_deassert();
  return TRUE;

fail:
  _sdcard_cs_deassert();
  return FALSE;
}

BOOL sdcard_writeBlock(uint32_t blockNumber, const uint8_t* data) {
  int16_t crc = 0xffff; // Dummy CRC value
  int i;

  // don't allow write to first block
  if (blockNumber == 0) {
    printf("SD_CARD_ERROR_WRITE_BLOCK_ZERO\n");
    goto fail;
  }

  // use address if not SDHC card
  if (_sdcard_type != SD_CARD_TYPE_SDHC) {
    blockNumber <<= 9;
  }

  if (_sdcard_command(CMD24, blockNumber)) {
    printf("SD_CARD_ERROR_CMD24\n");
    goto fail;
  }

  _sdcard_spi_transfer(DATA_START_BLOCK);
  for (i = 0; i < SDCARD_BLOCK_SIZE; i++) {
    _sdcard_spi_transfer(data[i]);
  }

  _sdcard_spi_transfer(crc >> 8);
  _sdcard_spi_transfer(crc);

  if ((_sdcard_spi_transfer(0xff) & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
    printf("SD_CARD_ERROR_WRITE\n");
    goto fail;
  }

  // wait for flash programming to complete
  if (!_sdcard_waitNotBusy(SD_WRITE_TIMEOUT)) {
    printf("SD_CARD_ERROR_WRITE_TIMEOUT\n");
    goto fail;
  }

  // response is r2 so get and check two bytes for nonzero
  if (_sdcard_command(CMD13, 0) || _sdcard_spi_transfer(0xff)) {
    printf("SD_CARD_ERROR_WRITE_PROGRAMMING\n");
    goto fail;
  }

  _sdcard_cs_deassert();
  return TRUE;

fail:
  _sdcard_cs_deassert();
  return FALSE;
}

uint8_t _sdcard_command(uint8_t cmd, uint32_t arg) {
  uint8_t status;

  _sdcard_cs_assert();

  _sdcard_waitNotBusy(SD_READ_TIMEOUT);

  // send command
  _sdcard_spi_transfer(cmd | 0x40);

  // send argument
  for (int8_t s = 24; s >= 0; s -= 8) {
    _sdcard_spi_transfer(arg >> s);
  }

  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) {
    crc = 0X95; // correct crc for CMD0 with arg 0
  }
  if (cmd == CMD8) {
    crc = 0X87; // correct crc for CMD8 with arg 0X1AA
  }
  _sdcard_spi_transfer(crc);

  // wait for response
  for (uint8_t i = 0; ((status = _sdcard_spi_transfer(0xff)) & 0X80) && i != 0XFF; i++);
  return status;
}

uint8_t _sdcard_acommand(uint8_t acmd, uint32_t arg) {
  _sdcard_command(CMD55, 0);
  return _sdcard_command(acmd, arg);
}

uint8_t _sdcard_spi_transfer(uint8_t d) {
  SPI_I2S_SendData(SDCARD_SPI, d);
  while (SPI_I2S_GetFlagStatus(SDCARD_SPI, SPI_I2S_FLAG_TXE) == RESET);
  while (SPI_I2S_GetFlagStatus(SDCARD_SPI, SPI_I2S_FLAG_RXNE) == RESET);
  while (SPI_I2S_GetFlagStatus(SDCARD_SPI, SPI_I2S_FLAG_BSY) == SET);
  return SPI_I2S_ReceiveData(SDCARD_SPI);
}

uint8_t _sdcard_waitStartBlock() {
  uint8_t status;
  uint16_t t0 = time_ms();
  while ((status = _sdcard_spi_transfer(0xff)) == 0XFF) {
    if (((uint16_t)time_ms() - t0) > SD_READ_TIMEOUT) {
      printf("SD_CARD_ERROR_READ_TIMEOUT\n");
      goto fail;
    }
  }
  if (status != DATA_START_BLOCK) {
    printf("SD_CARD_ERROR_READ\n");
    goto fail;
  }
  return TRUE;

fail:
  _sdcard_cs_deassert();
  return FALSE;
}

uint8_t _sdcard_waitNotBusy(uint16_t timeoutMillis) {
  uint16_t t0 = time_ms();
  do {
    if (_sdcard_spi_transfer(0xff) == 0XFF) {
      return TRUE;
    }
  } while (((uint16_t)time_ms() - t0) < timeoutMillis);
  return FALSE;
}

#endif
