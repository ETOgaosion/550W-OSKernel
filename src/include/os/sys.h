#pragma once

#include <common/types.h>

/*
 * Magic values required to use _reboot() system call.
 */

#define MOSS_REBOOT_MAGIC1 0xfee1dead
#define MOSS_REBOOT_MAGIC2 672274793
#define MOSS_REBOOT_MAGIC2A 85072278
#define MOSS_REBOOT_MAGIC2B 369367448
#define MOSS_REBOOT_MAGIC2C 537993216

/*
 * Commands accepted by the _reboot() system call.
 *
 * RESTART     Restart system using default command and mode.
 * HALT        Stop OS and give system control to ROM monitor, if any.
 * CAD_ON      Ctrl-Alt-Del sequence causes RESTART command.
 * CAD_OFF     Ctrl-Alt-Del sequence sends SIGINT to init task.
 * POWER_OFF   Stop OS and remove all power from system, if possible.
 * RESTART2    Restart system using given command string.
 * SW_SUSPEND  Suspend system using software suspend if compiled in.
 * KEXEC       Restart system using a previously loaded MOSS kernel
 */

#define MOSS_REBOOT_CMD_RESTART 0x01234567
#define MOSS_REBOOT_CMD_HALT 0xCDEF0123
#define MOSS_REBOOT_CMD_CAD_ON 0x89ABCDEF
#define MOSS_REBOOT_CMD_CAD_OFF 0x00000000
#define MOSS_REBOOT_CMD_POWER_OFF 0x4321FEDC
#define MOSS_REBOOT_CMD_RESTART2 0xA1B2C3D4
#define MOSS_REBOOT_CMD_SW_SUSPEND 0xD000FCE2
#define MOSS_REBOOT_CMD_KEXEC 0x45584543

/* Close the log.  Currently a NOP. */
#define SYSLOG_ACTION_CLOSE 0
/* Open the log. Currently a NOP. */
#define SYSLOG_ACTION_OPEN 1
/* Read from the log. */
#define SYSLOG_ACTION_READ 2
/* Read all messages remaining in the ring buffer. */
#define SYSLOG_ACTION_READ_ALL 3
/* Read and clear all messages remaining in the ring buffer */
#define SYSLOG_ACTION_READ_CLEAR 4
/* Clear ring buffer. */
#define SYSLOG_ACTION_CLEAR 5
/* Disable printk's to console */
#define SYSLOG_ACTION_CONSOLE_OFF 6
/* Enable printk's to console */
#define SYSLOG_ACTION_CONSOLE_ON 7
/* Set level of messages printed to console */
#define SYSLOG_ACTION_CONSOLE_LEVEL 8
/* Return number of unread characters in the log buffer */
#define SYSLOG_ACTION_SIZE_UNREAD 9
/* Return size of the log buffer */
#define SYSLOG_ACTION_SIZE_BUFFER 10

#define SYSLOG_FROM_READER 0
#define SYSLOG_FROM_PROC 1

#define SYSLOG_SIZE 4096

/* integer equivalents of KERN_<LEVEL> */

#define LOGLEVEL_SCHED -2   /* Deferred messages from sched code are set to this special level */
#define LOGLEVEL_DEFAULT -1 /* default (or last) loglevel */
#define LOGLEVEL_EMERG 0    /* system is unusable */
#define LOGLEVEL_ALERT 1    /* action must be taken immediately */
#define LOGLEVEL_CRIT 2     /* critical conditions */
#define LOGLEVEL_ERR 3      /* error conditions */
#define LOGLEVEL_WARNING 4  /* warning conditions */
#define LOGLEVEL_NOTICE 5   /* normal but significant condition */
#define LOGLEVEL_INFO 6     /* informational */
#define LOGLEVEL_DEBUG 7    /* debug-level messages */

extern char moss_log[SYSLOG_SIZE];

typedef struct uname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
} uname_t;

typedef struct sysinfo {
    kernel_long_t uptime;                                     /* Seconds since boot */
    kernel_ulong_t loads[3];                                  /* 1, 5, and 15 minute load averages */
    kernel_ulong_t totalram;                                  /* Total usable main memory size */
    kernel_ulong_t freeram;                                   /* Available memory size */
    kernel_ulong_t sharedram;                                 /* Amount of shared memory */
    kernel_ulong_t bufferram;                                 /* Memory used by buffers */
    kernel_ulong_t totalswap;                                 /* Total swap space size */
    kernel_ulong_t freeswap;                                  /* swap space still available */
    __u16 procs;                                              /* Number of current processes */
    __u16 pad;                                                /* Explicit padding for m68k */
    kernel_ulong_t totalhigh;                                 /* Total high memory size */
    kernel_ulong_t freehigh;                                  /* Available high memory size */
    __u32 mem_unit;                                           /* Memory unit size in bytes */
    char _f[20 - 2 * sizeof(kernel_ulong_t) - sizeof(__u32)]; /* Padding: libc5 uses this.. */
} sysinfo_t;

extern uname_t uname_550w;
extern sysinfo_t moss_info;

void k_sys_write_to_log(const char *log_msg);

long sys_uname(uname_t *);
long sys_gethostname(char *name, int len);
long sys_sethostname(char *name, int len);

long sys_syslog(int type, char *buf, int len);

long sys_reboot(int magic1, int magic2, unsigned int cmd, void *arg);

void k_sysinfo_init();
long sys_sysinfo(sysinfo_t *info);

// int kernel_start();
int kernel_restart();