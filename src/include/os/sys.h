#pragma once

#include <common/types.h>

typedef struct uname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
} uname_t;

typedef struct sysinfo {
    __kernel_long_t uptime;                                     /* Seconds since boot */
    __kernel_ulong_t loads[3];                                  /* 1, 5, and 15 minute load averages */
    __kernel_ulong_t totalram;                                  /* Total usable main memory size */
    __kernel_ulong_t freeram;                                   /* Available memory size */
    __kernel_ulong_t sharedram;                                 /* Amount of shared memory */
    __kernel_ulong_t bufferram;                                 /* Memory used by buffers */
    __kernel_ulong_t totalswap;                                 /* Total swap space size */
    __kernel_ulong_t freeswap;                                  /* swap space still available */
    __u16 procs;                                                /* Number of current processes */
    __u16 pad;                                                  /* Explicit padding for m68k */
    __kernel_ulong_t totalhigh;                                 /* Total high memory size */
    __kernel_ulong_t freehigh;                                  /* Available high memory size */
    __u32 mem_unit;                                             /* Memory unit size in bytes */
    char _f[20 - 2 * sizeof(__kernel_ulong_t) - sizeof(__u32)]; /* Padding: libc5 uses this.. */
} sysinfo_t;

long sys_uname(uname_t *);
long sys_gethostname(char *name, int len);
long sys_sethostname(char *name, int len);

long sys_syslog(int type, char *buf, int len);

long sys_reboot(int magic1, int magic2, unsigned int cmd, void *arg);

long sys_sysinfo(sysinfo_t *info);
