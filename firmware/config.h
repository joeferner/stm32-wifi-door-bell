
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stm32lib/util.h"

typedef struct {
  char ssid[40];
  char password[40];
  uint8_t security;
} ConfigWifi;

typedef struct {
  ConfigWifi wifi;
} Config;

extern Config config;

BOOL config_read();

#endif
