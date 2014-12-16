
#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "platform_config.h"

#ifdef SDCARD_ENABLE

#include "util.h"

BOOL sdcard_setup();
BOOL sdcard_read_block(uint32_t blockNumber, uint8_t* data);
BOOL sdcard_write_block(uint32_t blockNumber, const uint8_t* data);

#endif
#endif
