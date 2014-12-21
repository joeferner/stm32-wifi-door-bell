
#ifndef _SDCARD_FAT_H_
#define _SDCARD_FAT_H_

#include "platform_config.h"
#include "util.h"

#ifdef SDCARD_ENABLE

#include <stdint.h>
//#include <fcntl.h>

#ifndef SDCARD_PATH_PREFIX
# define SDCARD_PATH_PREFIX "/mnt/sdcard/"
#endif

#ifndef O_RDONLY
# define O_RDONLY   0x00000000
#endif
#ifndef O_WRONLY
# define O_WRONLY   0x00000001
#endif
#ifndef O_WRITE
# define O_WRITE      O_WRONLY
#endif
#ifndef O_RDWR
# define O_RDWR     0x00000002
#endif
#ifndef O_READ
# define O_READ       O_RDONLY
#endif
#ifndef O_ACCMODE
# define O_ACCMODE  0x00000003
#endif
#ifndef O_APPEND
# define O_APPEND   0x00000008
#endif
#ifndef O_SYNC
# define O_SYNC     0x00000080
#endif
#ifndef O_CREAT
# define O_CREAT    0x00000200
#endif
#ifndef O_TRUNC
# define O_TRUNC    0x00000400
#endif
#ifndef O_EXCL
# define O_EXCL     0x00000800
#endif

/** Default date for file timestamps is 1 Jan 2000 */
extern uint16_t const FAT_DEFAULT_DATE;

/** Default time for file timestamp is 1 am */
extern uint16_t const FAT_DEFAULT_TIME;

/** date field for FAT directory entry */
static inline uint16_t FAT_DATE(uint16_t year, uint8_t month, uint8_t day) {
  return (year - 1980) << 9 | month << 5 | day;
}

/** year part of FAT directory date field */
static inline uint16_t FAT_YEAR(uint16_t fatDate) {
  return 1980 + (fatDate >> 9);
}

/** month part of FAT directory date field */
static inline uint8_t FAT_MONTH(uint16_t fatDate) {
  return (fatDate >> 5) & 0XF;
}

/** day part of FAT directory date field */
static inline uint8_t FAT_DAY(uint16_t fatDate) {
  return fatDate & 0X1F;
}

/** time field for FAT directory entry */
static inline uint16_t FAT_TIME(uint8_t hour, uint8_t minute, uint8_t second) {
  return hour << 11 | minute << 5 | second >> 1;
}

/** hour part of FAT directory time field */
static inline uint8_t FAT_HOUR(uint16_t fatTime) {
  return fatTime >> 11;
}

/** minute part of FAT directory time field */
static inline uint8_t FAT_MINUTE(uint16_t fatTime) {
  return (fatTime >> 5) & 0X3F;
}

/** second part of FAT directory time field */
static inline uint8_t FAT_SECOND(uint16_t fatTime) {
  return 2 * (fatTime & 0X1F);
}

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
BOOL sdcard_fat_file_open(SdcardFatFile* f, const char* filePath, int mode);
void sdcard_fat_file_close(SdcardFatFile* f);
BOOL sdcard_fat_file_seek(SdcardFatFile* f, uint32_t pos);
uint32_t sdcard_fat_file_available(SdcardFatFile* f);
uint32_t sdcard_fat_file_size(SdcardFatFile* f);
uint32_t sdcard_fat_file_position(SdcardFatFile* f);
int16_t sdcard_fat_file_readByte(SdcardFatFile* f);
int16_t sdcard_fat_file_read(SdcardFatFile* f, uint8_t* buf, uint16_t nbyte);
int16_t sdcard_fat_file_write(SdcardFatFile* f, uint8_t* buf, uint16_t nbyte);

#endif
#endif
