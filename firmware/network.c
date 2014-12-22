
#include "network.h"
#include "platform_config.h"
#include <stdio.h>
#include "stm32lib/cc3000.h"
#include "stm32lib/delay.h"
#include "stm32lib/crc32.h"
#include "stm32lib/time.h"
#include "stm32lib/cc3000-host-driver/socket.h"
#include "config.h"

#define NETWORK_CONFIG_CHANGE_FILEPATH "/mnt/sdcard/network.sys"
#define BROADCAST_CONNECT_TIMEOUT 5000

uint32_t _network_ipAddress;

BOOL _network_displayConnectionInfo();
BOOL _network_displayMAC();
BOOL _network_hasConfigChanged();
void _network_writeConfigChangeFile();
BOOL _network_sendUdpBroadcast(int buttonNumber);

BOOL network_setup() {
  if (!cc3000_setup(FALSE, CC3000_CONNECT_POLICY_OPEN_AP | CC3000_CONNECT_POLICY_FAST | CC3000_CONNECT_POLICY_PROFILES, config.general.name)) {
    printf("failed cc3000 setup\n");
    return FALSE;
  }

  if (_network_hasConfigChanged()) {
    printf("network config has changed. writing new profiles to cc3000\n");

    if (cc3000_addProfile(config.wifi.ssid, config.wifi.password, config.wifi.security)) {
      printf("add profile complete\n");
    } else {
      printf("failed to add profile\n");
      cc3000_deleteAllProfiles();
      if (cc3000_addProfile(config.wifi.ssid, config.wifi.password, config.wifi.security)) {
        printf("add profile complete\n");
      } else {
        printf("failed to add profile\n");
        return FALSE;
      }
    }

    if (cc3000_finishAddProfileAndConnect()) {
      printf("connect with profile complete\n");
    } else {
      printf("failed to connect with profile\n");
      return FALSE;
    }

    _network_writeConfigChangeFile();
  }

  printf("Request DHCP\n");
  while (!cc3000_checkDHCP()) {
    delay_ms(100);
  }

  if (!_network_displayConnectionInfo()) {
    return FALSE;
  }
  delay_ms(10);

  return _network_displayMAC();
}

BOOL _network_displayConnectionInfo() {
  char buffer[20];
  uint32_t netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000_getIPAddress(&_network_ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    printf("Unable to retrieve the IP Address!\n");
    return FALSE;
  }

  printf("IP Addr: %s\n", cc3000_ipToString(_network_ipAddress, buffer));
  printf("Netmask: %s\n", cc3000_ipToString(netmask, buffer));
  printf("Gateway: %s\n", cc3000_ipToString(gateway, buffer));
  printf("DHCPsrv: %s\n", cc3000_ipToString(dhcpserv, buffer));
  printf("DNSserv: %s\n", cc3000_ipToString(dnsserv, buffer));
  return TRUE;
}

BOOL _network_displayMAC() {
  uint8_t macAddress[6];

  if (!cc3000_getMacAddress(macAddress)) {
    printf("Unable to retrieve the MAC address\n");
    return FALSE;
  }

  printf(
    "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
    macAddress[0],
    macAddress[1],
    macAddress[2],
    macAddress[3],
    macAddress[4],
    macAddress[5]
  );
  return TRUE;
}

BOOL _network_hasConfigChanged() {
  uint32_t fileCrc;
  FILE* f = fopen(NETWORK_CONFIG_CHANGE_FILEPATH, "r");
  if (f == NULL) {
    printf("network config could not open crc\n");
    return TRUE;
  }
  if (fread((uint8_t*)&fileCrc, 1, sizeof(fileCrc), f) < 4) {
    printf("network config could not read crc\n");
    fclose(f);
    return TRUE;
  }
  fclose(f);

  uint32_t crc = crc32(0, (uint8_t*)&config.wifi, sizeof(config.wifi));
  printf("network config %lx ?= %lx\n", crc, fileCrc);
  return (crc != fileCrc);
}

void _network_writeConfigChangeFile() {
  FILE* f = fopen(NETWORK_CONFIG_CHANGE_FILEPATH, "w");
  if (f == NULL) {
    printf("network could not write config change file\n");
    return;
  }

  uint32_t crc = crc32(0, (uint8_t*)&config.wifi, sizeof(config.wifi));
  fwrite((uint8_t*)&crc, 1, sizeof(crc), f);

  fclose(f);

  printf("network config wrote %lx\n", crc);
}

BOOL network_sendButtonEvent(int buttonNumber) {
  return _network_sendUdpBroadcast(buttonNumber);
}

BOOL _network_sendUdpBroadcast(int buttonNumber) {
  char buffer[100];
  SOCKET sock;
  sockaddr_in broadcastAddr, bindAddr;
  uint32_t broadcastIp = _network_ipAddress | 0x000000ff;

  int broadcastPort = 9898;
  printf("broadcasting to: %s:%d\n", cc3000_ipToString(broadcastIp, buffer), broadcastPort);

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("failed to create socket\n");
    return FALSE;
  }

  memset(&bindAddr, 0, sizeof(bindAddr));
  bindAddr.sin_family = AF_INET;
  bindAddr.sin_addr.s_addr = INADDR_ANY;
  bindAddr.sin_port = htons(broadcastPort);
  bind(sock, (const sockaddr*)&bindAddr, sizeof(bindAddr));

  sprintf(buffer, "{source: \"%s\", buttonNumber: %d}", config.general.name, buttonNumber);
  uint16_t bufferLen = strlen(buffer);

  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_addr.s_addr = htonl(broadcastIp);
  broadcastAddr.sin_port = htons(broadcastPort);
  if (sendto(sock, buffer, bufferLen, 0, (const sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) != bufferLen) {
    printf("send udp broadcast, failed write\n");
    closesocket(sock);
    return FALSE;
  }

  closesocket(sock);
  return TRUE;
}
