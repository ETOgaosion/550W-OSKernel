#pragma once

#include <lib/list.h>

#define TICKS_INTERVAL 100000
#define TIME_BASE_DEFAULT 10000000
#define TIME_INTERVAL TIME_BASE_DEFAULT / TICKS_INTERVAL

/*
 * Names of the interval timers, and structure
 * defining a timer setting:
 */
#define ITIMER_REAL 0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF 2

/*
 * The IDs of the various system clocks (for POSIX.1b interval timers):
 */
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define CLOCK_MONOTONIC_RAW 4
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#define CLOCK_BOOTTIME 7
#define CLOCK_REALTIME_ALARM 8
#define CLOCK_BOOTTIME_ALARM 9
/*
 * The driver implementing this got removed. The clock ID is kept as a
 * place holder. Do not reuse!
 */
#define CLOCK_SGI_CYCLE 10
#define CLOCK_TAI 11

#define MAX_CLOCKS 16
#define CLOCKS_MASK (CLOCK_REALTIME | CLOCK_MONOTONIC)
#define CLOCKS_MONO CLOCK_MONOTONIC

/*
 * Mode codes (timex.mode)
 */
#define ADJ_OFFSET 0x0001            /* time offset */
#define ADJ_FREQUENCY 0x0002         /* frequency offset */
#define ADJ_MAXERROR 0x0004          /* maximum time error */
#define ADJ_ESTERROR 0x0008          /* estimated time error */
#define ADJ_STATUS 0x0010            /* clock status */
#define ADJ_TIMECONST 0x0020         /* pll time constant */
#define ADJ_TAI 0x0080               /* set TAI offset */
#define ADJ_SETOFFSET 0x0100         /* add 'time' to current time */
#define ADJ_MICRO 0x1000             /* select microsecond resolution */
#define ADJ_NANO 0x2000              /* select nanosecond resolution */
#define ADJ_TICK 0x4000              /* tick value */
#define ADJ_OFFSET_SINGLESHOT 0x8001 /* old-fashioned adjtime */
#define ADJ_OFFSET_SS_READ 0xa001    /* read-only adjtime */

/*
 * Status codes (timex.status)
 */
#define STA_PLL 0x0001     /* enable PLL updates (rw) */
#define STA_PPSFREQ 0x0002 /* enable PPS freq discipline (rw) */
#define STA_PPSTIME 0x0004 /* enable PPS time discipline (rw) */
#define STA_FLL 0x0008     /* select frequency-lock mode (rw) */

#define STA_INS 0x0010      /* insert leap (rw) */
#define STA_DEL 0x0020      /* delete leap (rw) */
#define STA_UNSYNC 0x0040   /* clock unsynchronized (rw) */
#define STA_FREQHOLD 0x0080 /* hold frequency (rw) */

#define STA_PPSSIGNAL 0x0100 /* PPS signal present (ro) */
#define STA_PPSJITTER 0x0200 /* PPS signal jitter exceeded (ro) */
#define STA_PPSWANDER 0x0400 /* PPS signal wander exceeded (ro) */
#define STA_PPSERROR 0x0800  /* PPS signal calibration error (ro) */

#define STA_CLOCKERR 0x1000 /* clock hardware fault (ro) */
#define STA_NANO 0x2000     /* resolution (0 = us, 1 = ns) (ro) */
#define STA_MODE 0x4000     /* mode (0 = PLL, 1 = FLL) (ro) */
#define STA_CLK 0x8000      /* clock source (0 = A, 1 = B) (ro) */

/* read-only bits */
#define STA_RONLY (STA_PPSSIGNAL | STA_PPSJITTER | STA_PPSWANDER | STA_PPSERROR | STA_CLOCKERR | STA_NANO | STA_MODE | STA_CLK)

typedef struct time_val {
    kernel_time64_t sec; // 自 Unix 纪元起的秒数
    long long usec;      // 微秒数
} time_val_t;

#define kernel_timeval_t time_val_t

typedef struct nanotime_val {
    kernel_time64_t sec;  // 自 Unix 纪元起的秒数
    kernel_time64_t nsec; // 微秒数
} nanotime_val_t;

typedef struct kernel_timespec {
    kernel_time64_t tv_sec; /* seconds */
    long long tv_nsec;      /* nanoseconds */
} kernel_timespec_t;

typedef struct tms {
    kernel_clock_t tms_utime;
    kernel_clock_t tms_stime;
    kernel_clock_t tms_cutime;
    kernel_clock_t tms_cstime;
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

struct kernel_timex_timeval {
    kernel_time64_t tv_sec;
    long long tv_usec;
};

typedef struct kernel_timex {
    unsigned int modes;               /* mode selector */
    int : 32;                         /* pad */
    long long offset;                 /* time offset (usec) */
    long long freq;                   /* frequency offset (scaled ppm) */
    long long maxerror;               /* maximum error (usec) */
    long long esterror;               /* estimated error (usec) */
    int status;                       /* clock command/status */
    int : 32;                         /* pad */
    long long constant;               /* pll time constant */
    long long precision;              /* clock precision (usec) (read only) */
    long long tolerance;              /* clock frequency tolerance (ppm)
                                       * (read only)
                                       */
    struct kernel_timex_timeval time; /* (read only, except for ADJ_SETOFFSET) */
    long long tick;                   /* (modified) usecs between clock ticks */

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
} kernel_timex_t;

typedef struct kernel_itimerval {
    kernel_timeval_t it_interval;
    kernel_timeval_t it_value;
} kernel_itimerval_t;

typedef struct kernel_full_clock {
    kernel_timespec_t nano_clock;
    kernel_timex_t timex;
} kernel_full_clock_t;

typedef struct clock_set {
    kernel_full_clock_t real_time_clock;
    kernel_full_clock_t tai_clock;
    kernel_full_clock_t monotonic_clock;
    kernel_full_clock_t boot_time_clock;
} clock_set_t;

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern list_head timers;
extern kernel_timespec_t boot_time;
extern clock_set_t global_clocks;
extern kernel_timex_t timex;

void k_time_init();
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
int k_time_cmp_utime_0(time_val_t *time);

long sys_time(kernel_time_t *tloc);

long sys_getitimer(int which, kernel_itimerval_t *value);
long sys_setitimer(int which, kernel_itimerval_t *value, kernel_itimerval_t *ovalue);

long sys_clock_settime(clockid_t which_clock, const kernel_timespec_t *tp);

long sys_clock_gettime(clockid_t which_clock, kernel_timespec_t *tp);

long sys_clock_getres(clockid_t which_clock, kernel_timespec_t *tp);

long sys_times(tms_t *tbuf);

long sys_gettimeofday(time_val_t *tv, timezone_t *tz);

long sys_adjtimex(kernel_timex_t *txc_p);
long sys_clock_adjtime(clockid_t which_clock, kernel_timex_t *tx);
