
#include "ntp.h"
#include <stdio.h>
#include "platform_config.h"
#include "cc3000.h"
#include "time.h"
#include "rtc.h"
#include "cc3000-host-driver/socket.h"

#define LEAP_INDICATOR_UNKNOWN   0xc0

#define NTP_VERSION_3            0x18

#define NTP_MODE_CLIENT          0x03

#define SECONDS_FROM_1900_TO_1970 0x83AA7E80

typedef struct {
  uint32_t seconds;
  uint32_t fraction;
} _NTPTime;

typedef struct {
  uint8_t flags;
  uint8_t stratum;
  uint8_t poll_interval;
  uint8_t precision;
  uint32_t root_delay;
  uint32_t root_dispersion;
  uint32_t reference_id;
  _NTPTime reference_timestamp;
  _NTPTime origin_timestamp;
  _NTPTime receive_timestamp;
  _NTPTime transmit_timestamp;
} _NTPPacket;

uint32_t ntp_getTime() {
  char buffer[48];
  uint32_t ip, startTime, t = 0L;
  SOCKET sock;
  _NTPPacket ntpPacket;

  printf("Locating time server...\n");

  // Hostname to IP lookup; use NTP pool (rotates through servers)
#ifdef CC3000_ENABLE
  if (!gethostbyname(NTP_TIME_SERVER, strlen(NTP_TIME_SERVER), &ip)) {
    printf("Could not locate time server %s\n", NTP_TIME_SERVER);
    return NTP_FAIL;
  }

  printf("Attempting connection... %s\n", cc3000_ipToString(ip, buffer));
  startTime = time_ms();
  do {
    sock = cc3000_connectUDP(ip, 123);
  } while ((!cc3000_socket_connected(sock)) && ((time_ms() - startTime) < NTP_CONNECT_TIMEOUT));

  if (!cc3000_socket_connected(sock)) {
    return NTP_FAIL;
  }
  printf("connected!\nIssuing request...\n");

  // Assemble and issue request packet
  memset((uint8_t*)&ntpPacket, 0, sizeof(ntpPacket));
  ntpPacket.flags = LEAP_INDICATOR_UNKNOWN | NTP_VERSION_3 | NTP_MODE_CLIENT;
  ntpPacket.stratum = 0;
  ntpPacket.poll_interval = 10;
  ntpPacket.precision = 0xfa;
  ntpPacket.root_dispersion = 0;
  ntpPacket.transmit_timestamp.seconds = swapEndian(rtc_getSeconds() + SECONDS_FROM_1900_TO_1970);
  cc3000_socket_write(sock, (uint8_t*)&ntpPacket, sizeof(ntpPacket), 0);

  printf("Awaiting response...\n");
  memset((uint8_t*)&ntpPacket, 0, sizeof(ntpPacket));
  startTime = time_ms();
  while ((!cc3000_socket_available(sock)) && ((time_ms() - startTime) < NTP_RESPONSE_TIMEOUT));
  if (cc3000_socket_available(sock)) {
    cc3000_socket_read(sock, (uint8_t*)&ntpPacket, sizeof(ntpPacket), 0);
    t = swapEndian(ntpPacket.transmit_timestamp.seconds) - SECONDS_FROM_1900_TO_1970;
    printf("OK %lu\n", t);
  }
  cc3000_socket_close(sock);
  return t;

#else
# error Invalid driver
#endif
}
