
#include <stdint.h>
#include <string.h>
#include "util.h"

uint32_t swapEndian(uint32_t val) {
  return ((val << 24) & 0xff000000)
         | ((val << 8) & 0x00ff0000)
         | ((val >> 8) & 0x0000ff00)
         | ((val >> 24) & 0x000000ff);
}

void strTrim(char* str) {
  strTrimLeft(str);
  strTrimRight(str);
}

void strTrimLeft(char* str) {
  char* p = str;
  char* out = str;
  while (*p && isWhitespace(*p)) {
    p++;
  }
  while (*p) {
    *out++ = *p++;
  }
}

void strTrimRight(char* str) {
  char* p = str + strlen(str) - 1;
  while (p >= str && isWhitespace(*p)) {
    *p-- = '\0';
  }
}

int isWhitespace(char ch) {
  switch (ch) {
    case '\n':
    case '\r':
    case '\t':
    case ' ':
      return 1;
  }
  return 0;
}
