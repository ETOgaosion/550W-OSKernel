#pragma once

#include <common/errno.h>

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

#define __riscv_xlen 64

typedef unsigned __attribute__((__mode__(QI))) int8_t;
typedef unsigned __attribute__((__mode__(QI))) uint8_t;
typedef int __attribute__((__mode__(HI))) int16_t;
typedef unsigned __attribute__((__mode__(HI))) uint16_t;
typedef int __attribute__((__mode__(SI))) int32_t;
typedef unsigned __attribute__((__mode__(SI))) uint32_t;
typedef int __attribute__((__mode__(DI))) int64_t;
typedef unsigned __attribute__((__mode__(DI))) uint64_t;
typedef int __attribute__((__mode__(SI))) i32;

typedef int bool;

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned short wchar;

typedef int pid_t;
typedef uint64_t reg_t;
typedef uint64_t ptr_t;
typedef uint64_t clock_t;
typedef uint64_t sigval_t;
#if __riscv_xlen == 64
typedef int64 intptr_t;
typedef uint64 uintptr_t;
#elif __riscv_xlen == 32
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
#endif

/* size_t is used for memory object sizes */
typedef uintptr_t size_t;
typedef intptr_t ssize_t;
typedef uintptr_t ptrdiff_t;

typedef int8_t __s8;
typedef uint8_t __u8;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef int32_t __s32;
typedef uint32_t __u32;
typedef int64_t __s64;
typedef uint64_t __u64;

typedef __s8 s8;
typedef __u8 u8;
typedef __s16 s16;
typedef __u16 u16;
typedef __s32 s32;
typedef __u32 u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef uint8_t byte_size_t;

#ifndef kernel_long_t
typedef long kernel_long_t;
typedef unsigned long kernel_ulong_t;
#endif

#ifndef kernel_ino_t
typedef kernel_ulong_t kernel_ino_t;
#endif

#ifndef kernel_mode_t
typedef unsigned int kernel_mode_t;
#endif

#ifndef kernel_pid_t
typedef int kernel_pid_t;
#endif

#ifndef kernel_ipc_pid_t
typedef int kernel_ipc_pid_t;
#endif

#ifndef kernel_uid_t
typedef unsigned int kernel_uid_t;
typedef unsigned int kernel_gid_t;
#endif

#ifndef kernel_suseconds_t
typedef kernel_long_t kernel_suseconds_t;
#endif

#ifndef kernel_daddr_t
typedef int kernel_daddr_t;
#endif

#ifndef kernel_uid32_t
typedef unsigned int kernel_uid32_t;
typedef unsigned int kernel_gid32_t;
#endif

#ifndef kernel_dev_t
typedef unsigned int kernel_dev_t;
#endif

/*
 * Most 32 bit architectures use "unsigned int" size_t,
 * and all 64 bit architectures use "unsigned long" size_t.
 */
#ifndef kernel_size_t
#if __BITS_PER_LONG != 64
typedef unsigned int kernel_size_t;
typedef int kernel_ssize_t;
typedef int kernel_ptrdiff_t;
#else
typedef kernel_ulong_t kernel_size_t;
typedef kernel_long_t kernel_ssize_t;
typedef kernel_long_t kernel_ptrdiff_t;
#endif
#endif

#ifndef kernel_fsid_t
typedef struct kernel_fsid {
    int val[2];
} kernel_fsid_t;
#endif

typedef int kernel_key_t;
typedef int kernel_mqd_t;

/*
 * anything below here should be completely generic
 */
typedef kernel_long_t kernel_off_t;
typedef long long kernel_loff_t;
typedef kernel_long_t kernel_time_t;
typedef long long kernel_time64_t;
typedef kernel_long_t kernel_clock_t;
typedef int kernel_timer_t;
typedef int kernel_clockid_t;
typedef char *kernel_caddr_t;
typedef unsigned short kernel_uid16_t;
typedef unsigned short kernel_gid16_t;
typedef unsigned int kernel_uid32_t;
typedef unsigned int kernel_gid32_t;
typedef kernel_long_t kernel_old_time_t;
typedef kernel_long_t kernel_time_t;

typedef unsigned short umode_t;
typedef unsigned int mode_t;
typedef long int off_t;
typedef int32_t hartid_t;

typedef kernel_uid32_t uid_t;
typedef kernel_gid32_t gid_t;
typedef kernel_key_t key_t;
typedef kernel_clockid_t clockid_t;

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

#define ULONG_MAX (0xffffffffffffffffULL)
#define LONG_MAX (0x7fffffffffffffffLL)
#define INTMAX_MAX LONG_MAX
#define UINT_MAX (0xffffffffU)
#define INT_MAX (0x7fffffff)
#define UCHAR_MAX (0xffU)
#define CHAR_MAX (0x7f)

#define KB 0x1000
#define MB 0x1000000
#define GB 0x1000000000

#define readb(addr) (*(volatile uint8 *)(addr))
#define readw(addr) (*(volatile uint16 *)(addr))
#define readd(addr) (*(volatile uint32 *)(addr))
#define readq(addr) (*(volatile uint64 *)(addr))

/* clang-format off */
#define writeb(v, addr)                      \
    {                                        \
        (*(volatile uint8 *)(addr)) = (v); \
    }
#define writew(v, addr)                       \
    {                                         \
        (*(volatile uint16 *)(addr)) = (v); \
    }
#define writed(v, addr)                       \
    {                                         \
        (*(volatile uint32 *)(addr)) = (v); \
    }
#define writeq(v, addr)                       \
    {                                         \
        (*(volatile uint64 *)(addr)) = (v); \
    }

#define __DECLARE_FLEX_ARRAY(TYPE, NAME)	\
	struct { \
		struct { } __empty_ ## NAME; \
		TYPE NAME[]; \
	}
/* clang-format on */

#define DECLARE_FLEX_ARRAY(TYPE, NAME) __DECLARE_FLEX_ARRAY(TYPE, NAME)