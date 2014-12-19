
#ifndef _NTP_H_
#define _NTP_H_

#include <stdint.h>

#define NTP_FAIL ((uint32_t)-1)

#ifndef NTP_TIME_SERVER
# define NTP_TIME_SERVER "pool.ntp.org"
#endif

#ifndef NTP_CONNECT_TIMEOUT
# define NTP_CONNECT_TIMEOUT (15L * 1000L)
#endif

#ifndef NTP_RESPONSE_TIMEOUT
# define NTP_RESPONSE_TIMEOUT (15L * 1000L)
#endif

uint32_t ntp_getTime();

#endif
