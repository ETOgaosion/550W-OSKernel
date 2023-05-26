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

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern list_head timers;

uint64_t get_time_base();

long get_ticks(void);
void nano_u_time_converter(nanotime_val_t *nanotime, time_val_t *utime, bool direction);

void get_nanotime(nanotime_val_t *ntimebuf);
uint64_t get_ticks_from_nanotime(nanotime_val_t *ntimebuf);
void copy_nanotime(nanotime_val_t *src, nanotime_val_t *dst);
void minus_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res);
void add_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res);
int cmp_nanotime(nanotime_val_t *first, nanotime_val_t *sec);
void get_utime(time_val_t *ntimebuf);
uint64_t get_ticks_from_time(time_val_t *timebuf);
void copy_utime(time_val_t *src, time_val_t *dst);
void minus_utime(time_val_t *first, time_val_t *sec, time_val_t *res);
void add_utime(time_val_t *first, time_val_t *sec, time_val_t *res);
int cmp_utime(time_val_t *first, time_val_t *sec);

long sys_time(__kernel_time_t *tloc);
long sys_times(tms_t *tbuf);
long sys_gettimeofday(time_val_t *tv, timezone_t *tz);