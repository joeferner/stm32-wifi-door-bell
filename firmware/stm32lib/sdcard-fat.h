
#ifndef _SDCARD_FAT_H_
#define _SDCARD_FAT_H_

#include "platform_config.h"

#ifdef SDCARD_ENABLE

#include <stdint.h>
//#include <fcntl.h>

#ifndef O_ACCMODE
# define O_ACCMODE     0003
#endif
#ifndef O_RDONLY
# define O_RDONLY     00
#endif
#ifndef O_WRONLY
# define O_WRONLY     01
#endif
#ifndef O_RDWR
# define O_RDWR       02
#endif
#ifndef O_READ
# define O_READ       O_RDONLY
#endif
#ifndef O_WRITE
# define O_WRITE      O_WRONLY
#endif
#ifndef O_APPEND
# define O_APPEND     02000
#endif
#ifndef O_CREAT
# define O_CREAT      0100
#endif
#ifndef O_EXCL
# define O_EXCL       0200
#endif
#ifndef O_TRUNC
# define O_TRUNC      01000
#endif
#ifndef O_SYNC
# define O_SYNC       04010000
#endif

/** Default date for file timestamps is 1 Jan 2000 */
extern uint16_t const FAT_DEFAULT_DATE;

/** Default time for file timestamp is 1 am */
extern uint16_t const FAT_DEFAULT_TIME;

typedef struct {
  uint8_t type;
  uint32_t fileSize;
  uint32_t currentPosition;
  uint32_t currentCluster;
  uint32_t firstCluster;
  uint8_t dirIndex;
  uint32_t dirBlock;
  uint8_t flags;
  uint8_t attributes;
} SdcardFatFile;

extern void sdcard_fat_getTime(SdcardFatFile* f, uint16_t* creationDate, uint16_t* creationTime);

BOOL sdcard_fat_setup();
BOOL sdcard_fat_open(SdcardFatFile* f, const char* filePath, int mode);
void sdcard_fat_close(SdcardFatFile* f);
BOOL sdcard_fat_seek(SdcardFatFile* f, uint32_t pos);

#endif
#endif
