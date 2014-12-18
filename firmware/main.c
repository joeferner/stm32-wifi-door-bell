#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_rtc.h>
#include <misc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform_config.h"
#include "rtc.h"
#include "stm32lib/debug.h"
#include "stm32lib/delay.h"
#include "stm32lib/time.h"
#include "stm32lib/spi.h"
#include "stm32lib/sdcard.h"
#include "stm32lib/sdcard-fat.h"
#include "stm32lib/cc3000.h"

void setup();
void loop();

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
  time_setup();

  debug_setup();
  //rtc_setup();
  spi_setup();
  sdcard_setupGpio();
  cc3000_setupGpio();

  if (!sdcard_setup()) {
    printf("Failed to setup SDCard\n");
  } else {
    if (!sdcard_fat_setup()) {
      printf("Failed to setup SDCard Fat\n");
    }
  }

  char buffer[100];
  FILE* pFile = fopen("/mnt/sdcard/test.txt", "r");
  if (pFile == NULL) {
    printf("Failed to open test.txt\n");
    return;
  } else {
    printf("opened test.txt\n");
    while (fgets(buffer, sizeof(buffer), pFile) != NULL) {
      puts(buffer);
    }
    fclose(pFile);
    printf("closed test.txt\n");
  }

  if (!cc3000_setup(0, FALSE, MDNS_DEVICE_NAME)) {
    printf("failed cc3000 setup\n");
  }

#define WLAN_SSID       "WifiMonkey"
#define WLAN_PASS       "Ferner7037727884"
#define WLAN_SECURITY   WLAN_SEC_WPA2

  if (!cc3000_connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    printf("failed cc3000 connect to ap\n");
  }
}

void loop() {
  printf("loop\n");
  delay_ms(5000);
}

void sdcard_fat_getTime(SdcardFatFile* f, uint16_t* creationDate, uint16_t* creationTime) {
  uint32_t rtc = RTC_GetCounter();
  *creationDate = rtc / (24 * 60 * 60);
  *creationTime = FAT_DEFAULT_TIME;
}

void assert_failed(uint8_t* file, uint32_t line) {
  printf("!assert_failed: file %s on line %lu\n", (const char*)file, line);

  /* Infinite loop */
  while (1) {
  }
}

