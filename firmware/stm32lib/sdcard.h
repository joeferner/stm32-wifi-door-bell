
#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "platform_config.h"

#ifdef SDCARD_ENABLE

#include "util.h"

void sdcard_setupGpio();
BOOL sdcard_setup();
BOOL sdcard_readBlock(uint32_t blockNumber, uint8_t* data);
BOOL sdcard_writeBlock(uint32_t blockNumber, const uint8_t* data);

#endif
#endif
