
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

extern int  __HEAP_START;
extern int _write(int file, char* ptr, int len);

int _close(int file) {
  return -1;
}

int _fstat(int file, struct stat* st) {
  st->st_mode = S_IFCHR;
  return 0;
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
