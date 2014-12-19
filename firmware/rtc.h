
#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

void rtc_setup();
uint32_t rtc_getSeconds();
void rtc_setTime(uint32_t time);

#endif