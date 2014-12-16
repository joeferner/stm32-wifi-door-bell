#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <misc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform_config.h"
#include "stm32lib/debug.h"
#include "stm32lib/delay.h"
#include "stm32lib/time.h"
#include "stm32lib/spi.h"
#include "stm32lib/sdcard.h"
#include "stm32lib/sdcard-fat.h"

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
  spi_setup();
  sdcard_setup_gpio();
  
  if (!sdcard_setup()) {
    printf("Failed to setup SDCard\n");
  } else {
    if (!sdcard_fat_setup()) {
      printf("Failed to setup SDCard Fat\n");
    }
  }

  SDCARD_FAT_FILE f;
  if (!sdcard_fat_open(&f, "test.txt", SDCARD_FAT_READ)) {
    printf("Failed to open test.txt\n");
  } else {
    printf("opened test.txt\n");
    sdcard_fat_close(&f);
  }
}

void loop() {
  printf("loop\n");
  delay_ms(5000);
}

void assert_failed(uint8_t* file, uint32_t line) {
  printf("!assert_failed: file %s on line %lu\n", (const char*)file, line);

  /* Infinite loop */
  while (1) {
  }
}

