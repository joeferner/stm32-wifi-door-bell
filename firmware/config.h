
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stm32lib/util.h"

typedef struct {
  char name[40];
} ConfigGeneral;

typedef struct {
  char ssid[40];
  char password[40];
  uint8_t security;
} ConfigWifi;

typedef struct {
  char name[40];
} ConfigButton;

typedef struct {
  char baseUrl[40];
  char deviceId[40];
  char secret[40];
} ConfigCloud;

typedef struct {
  ConfigGeneral general;
  ConfigWifi wifi;
  ConfigButton button0;
  ConfigButton button1;
  ConfigCloud cloud;
} Config;

extern Config config;

BOOL config_read();

#endif
