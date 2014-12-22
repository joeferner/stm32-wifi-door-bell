
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "platform_config.h"

#ifdef BUTTON_ENABLE

#define BUTTON_NONE -1
#define BUTTON0      0
#define BUTTON1      1

void button_setup();
void button_loop();

void _button_irq(int buttonNumber);

#endif

#endif
