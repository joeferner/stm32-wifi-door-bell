
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"
#include "sdcard-fat.h"

extern int  __HEAP_START;

#ifdef SDCARD_ENABLE

#ifndef SDCARD_MAX_OPEN_FILES
# define SDCARD_MAX_OPEN_FILES 10
#endif
#ifndef SDCARD_START_FILE_DESCRIPTOR
# define SDCARD_START_FILE_DESCRIPTOR 1000
#endif

SdcardFatFile _newlib_sdcard_sdcardFatFiles[SDCARD_MAX_OPEN_FILES];
BOOL _newlib_sdcard_sdcardFatFilesUsed[SDCARD_MAX_OPEN_FILES] = {0};

int _newlib_sdcard_findAvailableFileSlot();
int _newlib_sdcard_open(const char* filename, int oflag, int pmode);
int _newlib_sdcard_read(int file, char* ptr, int len);
int _newlib_sdcard_write(int file, char* ptr, int len);
int _newlib_sdcard_close(int file);
int _newlib_sdcard_fstat(int file, struct stat* st);
int _newlib_sdcard_lseek(int file, int ptr, int dir);

#endif

#ifdef SDCARD_ENABLE
int _open(const char* filename, int oflag, int pmode) {
#ifdef SDCARD_ENABLE
  if (strncmp(filename, SDCARD_PATH_PREFIX, strlen(SDCARD_PATH_PREFIX)) == 0) {
    filename += strlen(SDCARD_PATH_PREFIX);
    return _newlib_sdcard_open(filename, oflag, pmode);
  }
#endif

  errno = ENOENT;
  return -1;
}
#endif

int _read(int file, char* ptr, int len) {
  int n;
  int num = 0;
  if (file == STDIN_FILENO) {
    for (n = 0; n < len; n++) {
      *ptr++ = (char)debug_read();
      num++;
    }
  }

#ifdef SDCARD_ENABLE
  else if (file >= SDCARD_START_FILE_DESCRIPTOR && file < (SDCARD_START_FILE_DESCRIPTOR + SDCARD_MAX_OPEN_FILES)) {
    return _newlib_sdcard_read(file, ptr, len);
  }
#endif

  else {
    errno = EBADF;
    return -1;
  }
  return num;
}

int _write(int file, char* ptr, int len) {
  int n;
  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    for (n = 0; n < len; n++) {
      debug_write(*ptr++);
    }
  }

#ifdef SDCARD_ENABLE
  else if (file >= SDCARD_START_FILE_DESCRIPTOR && file < (SDCARD_START_FILE_DESCRIPTOR + SDCARD_MAX_OPEN_FILES)) {
    return _newlib_sdcard_write(file, ptr, len);
  }
#endif

  else {
    errno = EBADF;
    return -1;
  }
  return len;
}

int _close(int file) {
#ifdef SDCARD_ENABLE
  if (file >= SDCARD_START_FILE_DESCRIPTOR && file < (SDCARD_START_FILE_DESCRIPTOR + SDCARD_MAX_OPEN_FILES)) {
    return _newlib_sdcard_close(file);
  }
#endif

  return -1;
}

int _fstat(int file, struct stat* st) {
  if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
    st->st_mode = S_IFCHR;
    return 0;
  }

#ifdef SDCARD_ENABLE
  if (file >= SDCARD_START_FILE_DESCRIPTOR && file < (SDCARD_START_FILE_DESCRIPTOR + SDCARD_MAX_OPEN_FILES)) {
    return _newlib_sdcard_fstat(file, st);
  }
#endif

  errno = EBADF;
  return -1;
}

int _isatty(int file) {
  switch (file) {
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
      return 1;
    default:
      //errno = ENOTTY;
      errno = EBADF;
      return 0;
  }
}

int _lseek(int file, int ptr, int dir) {
#ifdef SDCARD_ENABLE
  if (file >= SDCARD_START_FILE_DESCRIPTOR && file < (SDCARD_START_FILE_DESCRIPTOR + SDCARD_MAX_OPEN_FILES)) {
    return _newlib_sdcard_lseek(file, ptr, dir);
  }
#endif

  return 0;
}

caddr_t _sbrk(int incr) {
  static unsigned char* heap = NULL;
  unsigned char* prev_heap;

  if (heap == NULL) {
    heap = (unsigned char*)&__HEAP_START;
  }
  prev_heap = heap;
  /* check removed to show basic approach */

  heap += incr;

  return (caddr_t) prev_heap;
}

#ifdef SDCARD_ENABLE

#define SDCARD_GET                                                                        \
  BOOL used = _newlib_sdcard_sdcardFatFilesUsed[file - SDCARD_START_FILE_DESCRIPTOR];     \
  if (!used) {                                                                            \
    errno = EBADF;                                                                        \
    return -1;                                                                            \
  }                                                                                       \
  SdcardFatFile* f = &_newlib_sdcard_sdcardFatFiles[file - SDCARD_START_FILE_DESCRIPTOR];

int _newlib_sdcard_open(const char* filename, int oflag, int pmode) {
  int fileIndex = _newlib_sdcard_findAvailableFileSlot();
  if (fileIndex < 0) {
    errno = EMFILE;
    return -1;
  }
  SdcardFatFile* f = &_newlib_sdcard_sdcardFatFiles[fileIndex];
  if (!sdcard_fat_file_open(f, filename, oflag)) {
    errno = ENOENT;
    return -1;
  }
  _newlib_sdcard_sdcardFatFilesUsed[fileIndex] = TRUE;
  return SDCARD_START_FILE_DESCRIPTOR + fileIndex;
}

int _newlib_sdcard_read(int file, char* ptr, int len) {
  SDCARD_GET;
  return sdcard_fat_file_read(f, (uint8_t*)ptr, len);
}

int _newlib_sdcard_write(int file, char* ptr, int len) {
  SDCARD_GET;
  return sdcard_fat_file_write(f, (uint8_t*)ptr, len);
}

int _newlib_sdcard_close(int file) {
  SDCARD_GET;
  sdcard_fat_file_close(f);
  _newlib_sdcard_sdcardFatFilesUsed[file - SDCARD_START_FILE_DESCRIPTOR] = FALSE;
  return 0;
}

int _newlib_sdcard_fstat(int file, struct stat* st) {
  SDCARD_GET;
  st->st_size = f->fileSize;
  // TODO fill in other fields (date, etc)
  return 0;
}

int _newlib_sdcard_lseek(int file, int ptr, int dir) {
  SDCARD_GET;
  if (dir == SEEK_SET) {
    sdcard_fat_file_seek(f, ptr);
  } else if (dir == SEEK_CUR) {
    sdcard_fat_file_seek(f, f->currentPosition + ptr);
  } else if (dir == SEEK_END) {
    sdcard_fat_file_seek(f, f->fileSize + ptr);
  } else {
    errno = EINVAL;
    return -1;
  }

  return 0;
}

int _newlib_sdcard_findAvailableFileSlot() {
  for (int i = 0; i < SDCARD_MAX_OPEN_FILES; i++) {
    if (!_newlib_sdcard_sdcardFatFilesUsed[i]) {
      return i;
    }
  }
  return -1;
}

#endif
