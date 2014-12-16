
#ifndef _SDCARD_FAT_H_
#define _SDCARD_FAT_H_

#include "platform_config.h"

#ifdef SDCARD_ENABLE

#define SDCARD_FAT_READ 1

typedef struct {
  int i;
} SDCARD_FAT_FILE;

BOOL sdcard_fat_setup();
int sdcard_fat_open(SDCARD_FAT_FILE* f, const char* fileName, int access);
void sdcard_fat_close(SDCARD_FAT_FILE* f);

#endif
#endif