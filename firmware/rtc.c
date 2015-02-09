
#include "rtc.h"
#include <stdio.h>
#include "platform_config.h"

void rtc_setup() {
  HAL_RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  HAL_PWR_BackupAccessCmd(ENABLE);
  HAL_BKP_DeInit();

  HAL_RCC_LSEConfig(RCC_LSE_ON);
  while (HAL_RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  RCC_RTCCLKCmd(ENABLE);
  HAL_RTC_WaitForSynchro();
  HAL_RTC_WaitForLastTask();
  HAL_RTC_ITConfig(RTC_IT_SEC, ENABLE);
  HAL_RTC_WaitForLastTask();

  // set RTC period to 1sec
  HAL_RTC_SetPrescaler(32767); // RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
  HAL_RTC_WaitForLastTask();
}

void rtc_setTime(uint32_t time) {
  printf("BEGIN rtc_setTime %lu\n", time);
  HAL_PWR_BackupAccessCmd(ENABLE);
  HAL_RTC_WaitForLastTask();
  HAL_RTC_SetCounter(time);
  HAL_RTC_WaitForLastTask();
  printf("END rtc_setTime %lu\n", time);
}

uint32_t rtc_getSeconds() {
  return RTC_GetCounter();
}
