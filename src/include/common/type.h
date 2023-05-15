#pragma once

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef true
#define true 1
#define false 0
#endif

typedef unsigned __attribute__((__mode__(QI))) int8_t;
typedef unsigned __attribute__((__mode__(QI))) uint8_t;
typedef int __attribute__((__mode__(HI))) int16_t;
typedef unsigned __attribute__((__mode__(HI))) uint16_t;
typedef int __attribute__((__mode__(SI))) int32_t;
typedef unsigned __attribute__((__mode__(SI))) uint32_t;
typedef int __attribute__((__mode__(DI))) int64_t;
typedef unsigned __attribute__((__mode__(DI))) uint64_t;

typedef uint8_t bool;

typedef int32_t pid_t;
typedef uint64_t reg_t;
typedef uint64_t ptr_t;
typedef uint64_t uintptr_t;
typedef int64_t intptr_t;
typedef uint64_t size_t;
typedef uintptr_t ptrdiff_t;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Minimum of signed integral types.  */
#define INT8_MIN (-128)
#define INT16_MIN (-32767 - 1)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN (-9223372036854775807lu - 1)
/* Maximum of signed integral types.  */
#define INT8_MAX (127)
#define INT16_MAX (32767)
#define INT32_MAX (2147483647)
#define INT64_MAX (9223372036854775807lu)

/* Maximum of unsigned integral types.  */
#define UINT8_MAX (255)
#define UINT16_MAX (65535)
#define UINT32_MAX (4294967295U)
#define UINT64_MAX (18446744073709551615lu)
