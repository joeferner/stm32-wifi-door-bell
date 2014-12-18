
#ifndef UTIL_H
#define	UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef BOOL
# define BOOL  uint8_t
#endif
#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE  1
#endif

#ifndef ABS
# define ABS(a) ( ((a) < 0) ? -(a) : (a) )
#endif
#ifndef MIN
# define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#endif
#ifndef MAX
# define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef CLAMP
# define CLAMP(v, min, max) ( ((v) < (min)) ? (min) : ( ((v) > (max)) ? (max) : (v) ) )
#endif

uint32_t swapEndian(uint32_t val);
void strTrim(char* str);
void strTrimLeft(char* str);
void strTrimRight(char* str);
int isWhitespace(char ch);

#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

