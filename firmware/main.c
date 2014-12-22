#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_wwdg.h>
#include <misc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform_config.h"
#include "rtc.h"
#include "config.h"
#include "network.h"
#include "button.h"
#include "stm32lib/debug.h"
#include "stm32lib/delay.h"
#include "stm32lib/time.h"
#include "stm32lib/spi.h"
#include "stm32lib/sdcard.h"
#include "stm32lib/sdcard-fat.h"
#include "stm32lib/cc3000.h"
#include "stm32lib/ntp.h"

void setup();
void loop();
uint16_t rtcToFatTime(uint32_t t);
uint16_t rtcToFatDate(uint32_t t);

int main(void) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}

void setup() {
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  WWDG_DeInit();
  time_setup();

  debug_setup();
  rtc_setup();
  spi_setup();
  sdcard_setupGpio();
  cc3000_setupGpio();
  button_setup();

  if (!sdcard_setup()) {
    printf("Failed to setup SDCard\n");
  } else {
    if (!sdcard_fat_setup()) {
      printf("Failed to setup SDCard Fat\n");
    }
  }
  
  if (config_read()) {
    printf("read config success\n");
  } else {
    printf("read config FAILED\n");
    while (1);
  }

  network_setup();

  uint32_t ntpTime = ntp_getTime();
  printf("ntp time %lu\n", ntpTime);
  if (ntpTime > 0) {
    rtc_setTime(ntpTime);
  }
}

void loop() {
  button_loop();
}

void sdcard_fat_getTime(SdcardFatFile* f, uint16_t* creationDate, uint16_t* creationTime) {
  uint32_t rtc = rtc_getSeconds();
  *creationDate = rtcToFatDate(rtc);
  *creationTime = rtcToFatTime(rtc);
}

uint16_t rtcToFatTime(uint32_t t) {
  return (t % (24 * 60 * 60)) / 2;
}

uint16_t rtcToFatDate(uint32_t t) {
  static uint16_t DATE_FROM_1970_TO_1980 = 1387; // RTC is since 1970, FAT is since 1980.
  return (t / (24 * 60 * 60)) + DATE_FROM_1970_TO_1980;
}

void assert_failed(uint8_t* file, uint32_t line) {
  printf("!assert_failed: file %s on line %lu\n", (const char*)file, line);

  /* Infinite loop */
  while (1) {
  }
}

