
#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdint.h>
#include "platform_config.h"

void debug_setup();
#ifdef DEBUG_LED_ENABLE
void debug_led_set(int v);
#endif
uint8_t debug_read();
void debug_write(uint8_t b);

#endif
