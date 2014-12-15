#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <misc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform_config.h"
#include "debug.h"
#include "delay.h"
#include "time.h"

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

  debug_setup();
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

