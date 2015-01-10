
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "stm32lib/util.h"
#include "config.h"

BOOL network_setup();
BOOL network_sendButtonEvent(int buttonNumber, ConfigButton* configButton);

#endif
