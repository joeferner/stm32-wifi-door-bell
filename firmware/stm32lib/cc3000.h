
#ifndef _CC3000_H_
#define _CC3000_H_

#include "platform_config.h"

#ifdef CC3000_ENABLE

#include "util.h"
#include "cc3000-host-driver/cc3000_common.h"
#include "cc3000-host-driver/wlan.h"
#include "cc3000-host-driver/socket.h"
#include <string.h>

#ifndef MAX_SOCKETS
# define MAX_SOCKETS 4
#endif

#ifndef WLAN_CONNECT_TIMEOUT
# define WLAN_CONNECT_TIMEOUT 10000  /* how long to wait, in milliseconds */
#endif

#ifndef CC3000_RX_BUFFER_SIZE
# define CC3000_RX_BUFFER_SIZE 64
#endif

#define CC3000_CONNECT_POLICY_OPEN_AP       0x01
#define CC3000_CONNECT_POLICY_FAST          0x02
#define CC3000_CONNECT_POLICY_PROFILES      0x04
#define CC3000_CONNECT_POLICY_SMART_CONFIG  0x10

typedef int32_t SOCKET;

typedef void (*_cc3000_spiHandleRx)(void* p);
typedef void (*_cc3000_spiHandleTx)();

extern uint8_t wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];

void cc3000_setupGpio();
BOOL cc3000_setup(BOOL patchReq, uint8_t connectPolicy, const char* deviceName);
BOOL cc3000_connectToAP(const char* ssid, const char* key, uint8_t secmode);
BOOL cc3000_addProfile(const char* ssid, const char* key, uint8_t secmode);
BOOL cc3000_deleteAllProfiles();
BOOL cc3000_finishAddProfileAndConnect();
BOOL cc3000_checkDHCP();
BOOL cc3000_getIPAddress(uint32_t* retip, uint32_t* netmask, uint32_t* gateway, uint32_t* dhcpserv, uint32_t* dnsserv);
BOOL cc3000_getMacAddress(uint8_t* macAddress);
char* cc3000_ipToString(uint32_t ip, char* buffer);
SOCKET cc3000_connectUDP(uint32_t destIP, uint16_t destPort);

BOOL cc3000_socket_connected(SOCKET sock);
int cc3000_socket_available(SOCKET sock);
int cc3000_socket_write(SOCKET sock, const void* buf, uint16_t len, uint32_t flags);
int cc3000_socket_read(SOCKET sock, void* buf, uint16_t len, uint32_t flags);
BOOL cc3000_socket_close(SOCKET sock);

void _cc3000_irq();

#ifndef INADDR_NONE
# define INADDR_NONE ((uint32_t) 0xffffffff)
#endif
#ifndef INADDR_ANY
# define INADDR_ANY ((uint32_t) 0x00000000)
#endif

uint32_t inet_addr(const char* cp);
int inet_aton(const char* cp, in_addr* addr);

// used by CC3000 host driver
void _cc3000_spiOpen(_cc3000_spiHandleRx rxHandler);
void _cc3000_spiClose();
void _cc3000_irqPoll();
void _cc3000_spiResume();
void _cc3000_spiWrite(uint8_t* buffer, uint16_t length);

#endif
#endif
