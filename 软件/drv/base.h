#ifndef __BASE_H__
#define __BASE_H__

#include "config.h"

#include <stddef.h>

/*typedef signed char     int8_t;
typedef unsigned char   uint8_t;
typedef signed short    int16_t;
typedef unsigned short  uint16_t;
typedef signed long     int32_t;
typedef unsigned long   uint32_t;*/

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef volatile s32 vs32;
typedef volatile s16 vs16;
typedef volatile s8 vs8;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef volatile u32 vu32;
typedef volatile u16 vu16;
typedef volatile u8 vu8;

#ifndef INLINE
#define INLINE
#endif

#ifndef TRUE
typedef unsigned char   BOOL;
#define TRUE        1
#define FALSE       0
#endif

#ifndef NULL
#define NULL        ((void*)0)
#endif

#ifndef MIN
#define MIN(a,b)    ((a)<=(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b)    ((a)>=(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(v)      ((v)>0?(v):-(v))
#endif

#define BITS_SET(v,b)   ((v) |= (b))
#define BITS_CLR(v,b)   ((v) &= ~(b))
#define BITS_CHK(v,b)   (((v) & (b)) == (b))

#endif  // __BASE_H__
