
#include "rtc.h"
#include <stdio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_pwr.h>
#include <stm32f10x_bkp.h>

void rtc_setup() {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  BKP_DeInit();

  RCC_LSEConfig(RCC_LSE_ON);
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  RCC_RTCCLKCmd(ENABLE);
  RTC_WaitForSynchro();
  RTC_WaitForLastTask();
  RTC_ITConfig(RTC_IT_SEC, ENABLE);
  RTC_WaitForLastTask();

  // set RTC period to 1sec
  RTC_SetPrescaler(32767); // RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
  RTC_WaitForLastTask();
}

void rtc_setTime(uint32_t time) {
  printf("BEGIN rtc_setTime %lu\n", time);
  PWR_BackupAccessCmd(ENABLE);
  RTC_WaitForLastTask();
  RTC_SetCounter(time);
  RTC_WaitForLastTask();
  printf("END rtc_setTime %lu\n", time);
}

uint32_t rtc_getSeconds() {
  return RTC_GetCounter();
}
