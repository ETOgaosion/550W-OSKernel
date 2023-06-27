#pragma once

#include <lib/list.h>

#define TICKS_INTERVAL 100000
#define TIME_BASE_DEFAULT 10000000
#define TIME_INTERVAL TIME_BASE_DEFAULT / TICKS_INTERVAL

typedef struct time_val {
    __kernel_time64_t sec; // 自 Unix 纪元起的秒数
    long long usec;        // 微秒数
} time_val_t;

#define __kernel_timeval_t time_val_t

typedef struct nanotime_val {
    __kernel_time64_t sec;  // 自 Unix 纪元起的秒数
    __kernel_time64_t nsec; // 微秒数
} nanotime_val_t;

typedef struct __kernel_timespec {
    __kernel_time64_t tv_sec; /* seconds */
    long long tv_nsec;        /* nanoseconds */
} __kernel_timespec_t;

typedef struct tms {
    __kernel_clock_t tms_utime;
    __kernel_clock_t tms_stime;
    __kernel_clock_t tms_cutime;
    __kernel_clock_t tms_cstime;
} tms_t;

typedef struct timezone {
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of dst correction */
} timezone_t;

typedef struct pcbtimer {
    bool initialized;
    nanotime_val_t start_time;
    nanotime_val_t end_time;
    nanotime_val_t *remain_time;
} pcbtimer_t;

struct __kernel_timex_timeval {
    __kernel_time64_t tv_sec;
    long long tv_usec;
};

typedef struct __kernel_timex {
    unsigned int modes;                 /* mode selector */
    int : 32;                           /* pad */
    long long offset;                   /* time offset (usec) */
    long long freq;                     /* frequency offset (scaled ppm) */
    long long maxerror;                 /* maximum error (usec) */
    long long esterror;                 /* estimated error (usec) */
    int status;                         /* clock command/status */
    int : 32;                           /* pad */
    long long constant;                 /* pll time constant */
    long long precision;                /* clock precision (usec) (read only) */
    long long tolerance;                /* clock frequency tolerance (ppm)
                                         * (read only)
                                         */
    struct __kernel_timex_timeval time; /* (read only, except for ADJ_SETOFFSET) */
    long long tick;                     /* (modified) usecs between clock ticks */

    long long ppsfreq; /* pps frequency (scaled ppm) (ro) */
    long long jitter;  /* pps jitter (us) (ro) */
    int shift;         /* interval duration (s) (shift) (ro) */
    int : 32;          /* pad */
    long long stabil;  /* pps stability (scaled ppm) (ro) */
    long long jitcnt;  /* jitter limit exceeded (ro) */
    long long calcnt;  /* calibration intervals (ro) */
    long long errcnt;  /* calibration errors (ro) */
    long long stbcnt;  /* stability limit exceeded (ro) */

    int tai; /* TAI offset (ro) */

    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
    int : 32;
} __kernel_timex_t;

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern list_head timers;

uint64_t k_time_get_times_base();

long k_time_get_ticks(void);
void k_time_nano_u_time_converter(nanotime_val_t *nanotime, time_val_t *utime, bool direction);

void k_time_get_nanotime(nanotime_val_t *ntimebuf);
uint64_t k_time_get_ticks_from_nanotime(nanotime_val_t *ntimebuf);
void k_time_copy_nanotime(nanotime_val_t *src, nanotime_val_t *dst);
void k_time_minus_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res);
void k_time_add_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res);
int k_time_cmp_nanotime(nanotime_val_t *first, nanotime_val_t *sec);

void k_time_get_utime(time_val_t *ntimebuf);
uint64_t k_time_get_ticks_from_time(time_val_t *timebuf);
void k_time_copy_utime(time_val_t *src, time_val_t *dst);
void k_time_minus_utime(time_val_t *first, time_val_t *sec, time_val_t *res);
void k_time_add_utime(time_val_t *first, time_val_t *sec, time_val_t *res);
int k_time_cmp_utime(time_val_t *first, time_val_t *sec);

long sys_time(__kernel_time_t *tloc);

long sys_utimensat(int dfd, const char *filename, __kernel_timespec_t *utimes, int flags);

long sys_getitimer(int which, time_val_t *value);
long sys_setitimer(int which, time_val_t *value, time_val_t *ovalue);

long sys_clock_settime(clockid_t which_clock, const __kernel_timespec_t *tp);

long sys_clock_gettime(clockid_t which_clock, __kernel_timespec_t *tp);

long sys_clock_getres(clockid_t which_clock, __kernel_timespec_t *tp);

long sys_times(tms_t *tbuf);

long sys_gettimeofday(time_val_t *tv, timezone_t *tz);

long sys_adjtimex(__kernel_timex_t *txc_p);
long sys_clock_adjtime(clockid_t which_clock, __kernel_timex_t *tx);
