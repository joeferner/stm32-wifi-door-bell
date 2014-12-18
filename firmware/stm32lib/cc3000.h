
#ifndef _CC3000_H_
#define _CC3000_H_

#include "platform_config.h"

#ifdef CC3000_ENABLE

#include "util.h"
#include "cc3000-host-driver/cc3000_common.h"
#include "cc3000-host-driver/wlan.h"
#include <string.h>

#ifndef MAX_SOCKETS
# define MAX_SOCKETS 4
#endif

#ifndef WLAN_CONNECT_TIMEOUT
# define WLAN_CONNECT_TIMEOUT 10000  /* how long to wait, in milliseconds */
#endif

typedef void (*_cc3000_spiHandleRx)(void* p);
typedef void (*_cc3000_spiHandleTx)();

extern uint8_t wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

void cc3000_setupGpio();
BOOL cc3000_setup(uint8_t patchReq, BOOL useSmartConfigData, const char* deviceName);
BOOL cc3000_connectToAP(const char* ssid, const char* key, uint8_t secmode);
BOOL cc3000_checkDHCP();
BOOL cc3000_getIPAddress(uint32_t* retip, uint32_t* netmask, uint32_t* gateway, uint32_t* dhcpserv, uint32_t* dnsserv);
BOOL cc3000_getMacAddress(uint8_t* macAddress);
char* cc3000_ipToString(uint32_t ip, char* buffer);

void _cc3000_irq();

// used by CC3000 host driver
void _cc3000_spiOpen(_cc3000_spiHandleRx rxHandler);
void _cc3000_spiClose();
void _cc3000_irqPoll();
void _cc3000_spiResume();
void _cc3000_spiWrite(uint8_t* buffer, uint16_t length);

#endif
#endif
