#include <common/types.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <os/time.h>

uint32_t time_base = 0;
uint64_t time_elapsed = 0;
list_head timers;
kernel_timespec_t boot_time;
clock_set_t global_clocks;
kernel_timex_t timex;
uint64_t boot_ticks;

timezone_t timezone_550W = {.tz_minuteswest = -480, .tz_dsttime = 0};

void k_time_init() {
    time_base = TIME_BASE_DEFAULT;
    k_time_get_nanotime((nanotime_val_t *)&boot_time);
    k_memcpy(&global_clocks.real_time_clock, &boot_time, sizeof(clock_t));
    k_memcpy(&global_clocks.tai_clock, &boot_time, sizeof(clock_t));
    k_bzero((void *)&global_clocks.monotonic_clock, sizeof(clock_t));
    k_bzero((void *)&global_clocks.boot_time_clock, sizeof(clock_t));
    k_bzero((void *)&timex, sizeof(kernel_timex_t));
    boot_ticks = k_time_get_ticks();
}

long k_time_get_ticks() {
    __asm__ __volatile__("rdtime %0" : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer() {
    return k_time_get_ticks() / time_base;
}

uint64_t k_time_get_times_base() {
    return time_base;
}

void k_time_nano_u_time_converter(nanotime_val_t *nanotime, time_val_t *utime, bool direction) {
    if (direction) {
        utime->sec = nanotime->sec;
        utime->usec = nanotime->nsec * 1000;
    } else {
        nanotime->sec = utime->sec;
        nanotime->nsec = utime->usec / 1000;
    }
}

void k_time_get_nanotime(nanotime_val_t *ntimebuf) {
    long ticks = k_time_get_ticks();
    ntimebuf->sec = ticks / time_base;
    ntimebuf->nsec = ((ticks % time_base) * 1000000000) / time_base;
}

uint64_t k_time_get_ticks_from_nanotime(nanotime_val_t *ntimebuf) {
    uint64_t ticks = (ntimebuf->sec * 1000000000 + ntimebuf->nsec) * 1000000000 / time_base;
    return ticks;
}

void k_time_copy_nanotime(nanotime_val_t *dst, nanotime_val_t *src) {
    dst->sec = src->sec;
    dst->nsec = src->nsec;
}

void k_time_minus_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res) {
    if (first->nsec < sec->nsec) {
        if (!res) {
            first->sec = first->sec - sec->sec - 1;
            first->nsec = first->nsec + 1000000000 - sec->nsec;
            return;
        }
        res->sec = first->sec - sec->sec - 1;
        res->nsec = first->nsec + 1000000000 - sec->nsec;
    } else {
        if (!res) {
            first->sec = first->sec - sec->sec;
            first->nsec = first->nsec - sec->nsec;
            return;
        }
        res->sec = first->sec - sec->sec;
        res->nsec = first->nsec - sec->nsec;
    }
}

void k_time_add_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res) {
    int nsec = first->nsec + sec->nsec;
    if (!res) {
        first->nsec = nsec % (int)1000000000;
        first->sec = first->sec + sec->sec + nsec / (int)1000000000;
        return;
    }
    res->nsec = nsec % (int)1000000000;
    res->sec = first->sec + sec->sec + nsec / (int)1000000000;
}

int k_time_cmp_nanotime(nanotime_val_t *first, nanotime_val_t *sec) {
    if (first->sec == sec->sec) {
        if (first->nsec > sec->nsec) {
            return 1;
        } else if (first->nsec == sec->nsec) {
            return 0;
        } else {
            return -1;
        }
    } else if (first->sec > sec->sec) {
        return 1;
    } else {
        return -1;
    }
}

void k_time_get_utime(time_val_t *utimebuf) {
    long ticks = k_time_get_ticks();
    utimebuf->sec = ticks / time_base;
    utimebuf->usec = ((ticks % time_base) * 1000000) / time_base;
}

uint64_t k_time_get_ticks_from_time(time_val_t *ntimebuf) {
    uint64_t ticks = (ntimebuf->sec * 1000000 + ntimebuf->usec) * 1000000 / time_base;
    return ticks;
}

void k_time_copy_utime(time_val_t *src, time_val_t *dst) {
    dst->sec = src->sec;
    dst->usec = src->usec;
}

void k_time_minus_utime(time_val_t *first, time_val_t *sec, time_val_t *res) {
    if (first->usec < sec->usec) {
        if (!res) {
            first->sec = first->sec - sec->sec - 1;
            first->usec = first->usec + 1000000000 - sec->usec;
            return;
        }
        res->sec = first->sec - sec->sec - 1;
        res->usec = first->usec + 1000000000 - sec->usec;
    } else {
        if (!res) {
            first->sec = first->sec - sec->sec;
            first->usec = first->usec - sec->usec;
            return;
        }
        res->sec = first->sec - sec->sec;
        res->usec = first->usec - sec->usec;
    }
}

void k_time_add_utime(time_val_t *first, time_val_t *sec, time_val_t *res) {
    int usec = first->usec + sec->usec;
    if (!res) {
        first->usec = usec % (int)1000000;
        first->sec = first->sec + sec->sec + usec / (int)1000000;
        return;
    }
    res->usec = usec % (int)1000000;
    res->sec = first->sec + sec->sec + usec / (int)1000000;
}

int k_time_cmp_utime(time_val_t *first, time_val_t *sec) {
    if (first->sec == sec->sec) {
        if (first->usec > sec->usec) {
            return 1;
        } else if (first->usec == sec->usec) {
            return 0;
        } else {
            return -1;
        }
    } else if (first->sec > sec->sec) {
        return 1;
    } else {
        return -1;
    }
}

int k_time_cmp_utime_0(time_val_t *time) {
    time_val_t sec = {.sec = 0, .usec = 0};
    return k_time_cmp_utime(time, &sec);
}

long sys_time(kernel_time_t *tloc) {
    long ret = get_timer();
    *tloc = ret;
    return ret;
}

// [TODO]
long sys_getitimer(int which, kernel_itimerval_t *value) {
    switch (which) {
    case ITIMER_REAL:
        k_memcpy(value, &(*current_running)->real_itime, sizeof(kernel_itimerval_t));
        break;
    case ITIMER_VIRTUAL:
        k_memcpy(value, &(*current_running)->virt_itime, sizeof(kernel_itimerval_t));
        break;
    case ITIMER_PROF:
        k_memcpy(value, &(*current_running)->prof_itime, sizeof(kernel_itimerval_t));
        break;

    default:
        break;
    }
    return 0;
}

long sys_setitimer(int which, kernel_itimerval_t *value, kernel_itimerval_t *ovalue) {
    switch (which) {
    case ITIMER_REAL:
        k_memcpy(&(*current_running)->real_itime, value, sizeof(kernel_itimerval_t));
        if (ovalue) {
            k_memcpy(ovalue, &(*current_running)->real_itime, sizeof(kernel_itimerval_t));
        }
        break;
    case ITIMER_VIRTUAL:
        k_memcpy(&(*current_running)->virt_itime, value, sizeof(kernel_itimerval_t));
        if (ovalue) {
            k_memcpy(ovalue, &(*current_running)->virt_itime, sizeof(kernel_itimerval_t));
        }
        break;
    case ITIMER_PROF:
        k_memcpy(&(*current_running)->prof_itime, value, sizeof(kernel_itimerval_t));
        if (ovalue) {
            k_memcpy(ovalue, &(*current_running)->prof_itime, sizeof(kernel_itimerval_t));
        }
        break;

    default:
        break;
    }
    return 0;
}

long sys_clock_settime(clockid_t which_clock, const kernel_timespec_t *tp) {
    if (which_clock != CLOCK_REALTIME) {
        return -EINVAL;
    }
    global_clocks.real_time_clock.nano_clock.tv_sec = tp->tv_sec;
    global_clocks.real_time_clock.nano_clock.tv_nsec = tp->tv_nsec;
    return 0;
}

long sys_clock_gettime(clockid_t which_clock, kernel_timespec_t *tp) {
    switch (which_clock) {
    case CLOCK_REALTIME:
    case CLOCK_REALTIME_ALARM:
    case CLOCK_REALTIME_COARSE:
        tp->tv_sec = global_clocks.real_time_clock.nano_clock.tv_sec;
        tp->tv_nsec = global_clocks.real_time_clock.nano_clock.tv_nsec;
        break;
    case CLOCK_TAI:
        tp->tv_sec = global_clocks.tai_clock.nano_clock.tv_sec;
        tp->tv_nsec = global_clocks.tai_clock.nano_clock.tv_nsec;
        break;
    case CLOCK_MONOTONIC:
    case CLOCK_MONOTONIC_COARSE:
    case CLOCK_MONOTONIC_RAW:
        tp->tv_sec = global_clocks.monotonic_clock.nano_clock.tv_sec;
        tp->tv_nsec = global_clocks.monotonic_clock.nano_clock.tv_nsec;
        break;
    case CLOCK_BOOTTIME:
    case CLOCK_BOOTTIME_ALARM:
        tp->tv_sec = global_clocks.boot_time_clock.nano_clock.tv_sec;
        tp->tv_nsec = global_clocks.boot_time_clock.nano_clock.tv_nsec;
        break;
    case CLOCK_PROCESS_CPUTIME_ID:
        if ((*current_running)->type == USER_THREAD) {
            return -EINVAL;
        }
        tp->tv_sec = (*current_running)->cputime_id_clock.nano_clock.tv_sec;
        tp->tv_nsec = (*current_running)->cputime_id_clock.nano_clock.tv_nsec;
        break;
    case CLOCK_THREAD_CPUTIME_ID:
        if ((*current_running)->type == USER_PROCESS) {
            return -EINVAL;
        }
        tp->tv_sec = (*current_running)->cputime_id_clock.nano_clock.tv_sec;
        tp->tv_nsec = (*current_running)->cputime_id_clock.nano_clock.tv_nsec;
        break;

    default:
        break;
    }
    return 0;
}

// [NOT CLEAR]
long sys_clock_getres(clockid_t which_clock, kernel_timespec_t *tp) {
    tp->tv_sec = 0;
    tp->tv_nsec = 100000;
    return 0;
}

long sys_times(tms_t *tbuf) {
    tbuf->tms_stime = k_time_get_ticks_from_time(&(*current_running)->resources.ru_stime);
    tbuf->tms_utime = k_time_get_ticks_from_time(&(*current_running)->resources.ru_utime);
    tbuf->tms_cstime = (*current_running)->dead_child_stime;
    tbuf->tms_cutime = (*current_running)->dead_child_utime;
    for (int i = 0; i < NUM_MAX_CHILD; i++) {
        if ((*current_running)->child_pids[i] == 0) {
            continue;
        }
        tbuf->tms_cstime += k_time_get_ticks_from_time(&pcb[(*current_running)->child_pids[i]].resources.ru_stime);
        tbuf->tms_cutime += k_time_get_ticks_from_time(&pcb[(*current_running)->child_pids[i]].resources.ru_utime);
    }
    return k_time_get_ticks();
}

long sys_gettimeofday(time_val_t *tv, timezone_t *tz) {
    if (tv) {
        k_time_get_utime(tv);
    }
    if (tz) {
        tz->tz_minuteswest = timezone_550W.tz_minuteswest;
        tz->tz_dsttime = timezone_550W.tz_dsttime;
    }
    return 0;
}

void adjtimex(kernel_timex_t *target, kernel_timex_t *txc_p) {
    if (txc_p->modes | ADJ_OFFSET) {
        target->offset = txc_p->offset;
    } else if (txc_p->modes | ADJ_FREQUENCY) {
        target->offset = txc_p->freq;
    } else if (txc_p->modes | ADJ_MAXERROR) {
        target->maxerror = txc_p->maxerror;
    } else if (txc_p->modes | ADJ_ESTERROR) {
        target->esterror = txc_p->esterror;
    } else if (txc_p->modes | ADJ_STATUS) {
        target->status |= (txc_p->status & (STA_PLL | STA_PPSERROR | STA_PPSTIME | STA_FLL | STA_INS | STA_DEL | STA_INS | STA_UNSYNC | STA_FREQHOLD));
    } else if (txc_p->modes | ADJ_TIMECONST) {
        target->constant = txc_p->constant;
    } else if (txc_p->modes | ADJ_TAI) {
        target->tai = txc_p->tai;
    } else if (txc_p->modes | ADJ_SETOFFSET) {
        k_time_add_nanotime((nanotime_val_t *)&target->time, (nanotime_val_t *)&txc_p->time, NULL);
    } else if (txc_p->modes | ADJ_MICRO) {
        target->offset = target->tick;
    } else if (txc_p->modes | ADJ_NANO) {
        target->offset = target->tick * 1000;
    } else if (txc_p->modes | ADJ_TICK) {
        target->tick = txc_p->tick;
    }
    k_memcpy(txc_p, &timex, sizeof(kernel_timex_t));
}

long sys_adjtimex(kernel_timex_t *txc_p) {
    adjtimex(&timex, txc_p);
    return 0;
}

long sys_clock_adjtime(clockid_t which_clock, kernel_timex_t *tx) {
    switch (which_clock) {
    case CLOCK_REALTIME:
    case CLOCK_REALTIME_ALARM:
    case CLOCK_REALTIME_COARSE:
        adjtimex(&global_clocks.real_time_clock.timex, tx);
        break;
    case CLOCK_TAI:
        adjtimex(&global_clocks.tai_clock.timex, tx);
        break;
    case CLOCK_MONOTONIC:
    case CLOCK_MONOTONIC_COARSE:
    case CLOCK_MONOTONIC_RAW:
        adjtimex(&global_clocks.monotonic_clock.timex, tx);
        break;
    case CLOCK_BOOTTIME:
    case CLOCK_BOOTTIME_ALARM:
        adjtimex(&global_clocks.boot_time_clock.timex, tx);
        break;
    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
        adjtimex(&(*current_running)->cputime_id_clock.timex, tx);
        break;

    default:
        break;
    }
    return 0;
}

long sys_clock_nanosleep(clockid_t which_clock, int flags, const kernel_timespec_t *rqtp, kernel_timespec_t *rmtp) {
    return sys_nanosleep((nanotime_val_t *)rqtp, (nanotime_val_t *)rmtp);
}