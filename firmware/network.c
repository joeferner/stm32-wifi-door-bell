
#include "network.h"
#include "platform_config.h"
#include <stdio.h>
#include "stm32lib/cc3000.h"
#include "stm32lib/delay.h"
#include "config.h"

BOOL _network_displayConnectionInfo();

BOOL network_setup() {
  if (!cc3000_setup(0, FALSE, MDNS_DEVICE_NAME)) {
    printf("failed cc3000 setup\n");
    return FALSE;
  }

  if (cc3000_connectToAP(config.wifi.ssid, config.wifi.password, config.wifi.security)) {
    printf("Connected\n");
  } else {
    printf("failed cc3000 connect to ap\n");
    return FALSE;
  }

  printf("Request DHCP\n");
  while (!cc3000_checkDHCP()) {
    delay_ms(100);
  }

  return _network_displayConnectionInfo();
}

BOOL _network_displayConnectionInfo() {
  char buffer[20];
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000_getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    printf("Unable to retrieve the IP Address!\n");
    return FALSE;
  } else {
    printf("IP Addr: %s\n", cc3000_ipToString(ipAddress, buffer));
    printf("Netmask: %s\n", cc3000_ipToString(netmask, buffer));
    printf("Gateway: %s\n", cc3000_ipToString(gateway, buffer));
    printf("DHCPsrv: %s\n", cc3000_ipToString(dhcpserv, buffer));
    printf("DNSserv: %s\n", cc3000_ipToString(dnsserv, buffer));
    return TRUE;
  }
}

