#pragma once

#include <common/types.h>
#include <os/time.h>

#define CPU_NUM 2
#define CPU_SET_SIZE 1

/*
 * Resource limit IDs
 *
 * ( Compatibility detail: there are architectures that have
 *   a different rlimit ID order in the 5-9 range and want
 *   to keep that order for binary compatibility. The reasons
 *   are historic and all new rlimits are identical across all
 *   arches. If an arch has such special order for some rlimits
 *   then it defines them prior including asm-generic/resource.h. )
 */

#define RLIMIT_CPU 0   /* CPU time in sec */
#define RLIMIT_FSIZE 1 /* Maximum filesize */
#define RLIMIT_DATA 2  /* max data size */
#define RLIMIT_STACK 3 /* max stack size */
#define RLIMIT_CORE 4  /* max core file size */

#define RLIMIT_RSS 5 /* max resident set size */

#define RLIMIT_NPROC 6 /* max number of processes */

#define RLIMIT_NOFILE 7 /* max number of open files */

#define RLIMIT_MEMLOCK 8 /* max locked-in-memory address space */

#define RLIMIT_AS 9 /* address space limit */

#define RLIMIT_LOCKS 10      /* maximum file locks held */
#define RLIMIT_SIGPENDING 11 /* max number of pending signals */
#define RLIMIT_MSGQUEUE 12   /* maximum bytes in POSIX mqueues */
#define RLIMIT_NICE 13       /* max nice prio allowed to raise to 0-39 for nice level 19 .. -20 */
#define RLIMIT_RTPRIO 14     /* maximum realtime priority */
#define RLIMIT_RTTIME 15     /* timeout for RT tasks in us */
#define RLIM_NLIMITS 16

/*
 * SuS says limits have to be unsigned.
 * Which makes a ton more sense anyway.
 *
 * Some architectures override this (for compatibility reasons):
 */
#define RLIM_INFINITY (~0UL)

typedef struct rusage {
    kernel_timeval_t ru_utime; /* user time used */
    kernel_timeval_t ru_stime; /* system time used */
    kernel_long_t ru_maxrss;   /* maximum resident set size */
    kernel_long_t ru_ixrss;    /* integral shared memory size */
    kernel_long_t ru_idrss;    /* integral unshared data size */
    kernel_long_t ru_isrss;    /* integral unshared stack size */
    kernel_long_t ru_minflt;   /* page reclaims */
    kernel_long_t ru_majflt;   /* page faults */
    kernel_long_t ru_nswap;    /* swaps */
    kernel_long_t ru_inblock;  /* block input operations */
    kernel_long_t ru_oublock;  /* block output operations */
    kernel_long_t ru_msgsnd;   /* messages sent */
    kernel_long_t ru_msgrcv;   /* messages received */
    kernel_long_t ru_nsignals; /* signals received */
    kernel_long_t ru_nvcsw;    /* voluntary context switches */
    kernel_long_t ru_nivcsw;   /* involuntary " */
} rusage_t;

typedef struct rlimit {
    kernel_ulong_t rlim_cur;
    kernel_ulong_t rlim_max;
} rlimit_t;

typedef struct rlimit64 {
    __u64 rlim_cur;
    __u64 rlim_max;
} rlimit64_t;

typedef rlimit_t all_resources_t[RLIM_NLIMITS];

extern all_resources_t moss_resources;

void k_resources_init();
void k_set_rlimit(rlimit_t *target, kernel_ulong_t cur, kernel_ulong_t max);

long sys_getrlimit(unsigned int resource, rlimit_t *rlim);
long sys_setrlimit(unsigned int resource, rlimit_t *rlim);
