#include <common/types.h>
#include <lib/list.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <os/time.h>

uint32_t time_base = 0;
uint64_t time_elapsed = 0;
list_head timers;

timezone_t timezone_550W = {.tz_minuteswest = -480, .tz_dsttime = 0};

long get_ticks() {
    __asm__ __volatile__("rdtime %0" : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer() { return get_ticks() / time_base; }

uint64_t get_time_base() { return time_base; }

void nano_u_time_converter(nanotime_val_t *nanotime, time_val_t *utime, bool direction) {
    if (direction) {
        utime->sec = nanotime->sec;
        utime->usec = nanotime->nsec * 1e3;
    } else {
        nanotime->sec = utime->sec;
        nanotime->nsec = utime->usec / 1e3;
    }
}

void get_nanotime(nanotime_val_t *ntimebuf) {
    long ticks = get_ticks();
    ntimebuf->sec = ticks / time_base;
    ntimebuf->nsec = ((ticks % time_base) * 1e9) / time_base;
}

void copy_nanotime(nanotime_val_t *dst, nanotime_val_t *src) {
    dst->sec = src->sec;
    dst->nsec = src->nsec;
}

void minus_nanotime(nanotime_val_t *first, nanotime_val_t *sec, nanotime_val_t *res) {
    if (first->nsec < sec->nsec) {
        res->sec = first->sec - sec->sec - 1;
        res->nsec = first->nsec + 1e9 - sec->nsec;
    } else {
        res->sec = first->sec - sec->sec;
        res->nsec = first->nsec - sec->nsec;
    }
}

int cmp_nanotime(nanotime_val_t *first, nanotime_val_t *sec) {
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

void get_utime(time_val_t *utimebuf) {
    long ticks = get_ticks();
    utimebuf->sec = ticks / time_base;
    utimebuf->usec = ((ticks % time_base) * 1e6) / time_base;
}

void copy_utime(time_val_t *dst, time_val_t *src) {
    dst->sec = src->sec;
    dst->usec = src->usec;
}

void minus_utime(time_val_t *first, time_val_t *sec, time_val_t *res) {
    if (first->usec < sec->usec) {
        res->sec = first->sec - sec->sec - 1;
        res->usec = first->usec + 1e9 - sec->usec;
    } else {
        res->sec = first->sec - sec->sec;
        res->usec = first->usec - sec->usec;
    }
}

int cmp_utime(time_val_t *first, time_val_t *sec) {
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

long sys_time(__kernel_time_t *tloc) {
    long ret = get_timer();
    *tloc = ret;
    return ret;
}

long sys_times(tms_t *tbuf) {
    tbuf->tms_stime = (*current_running)->stime;
    tbuf->tms_utime = (*current_running)->utime;
    for (int i = 0; i < (*current_running)->child_num; i++) {
        tbuf->tms_cstime += pcb[(*current_running)->child_pid[i]].stime;
        tbuf->tms_cutime += pcb[(*current_running)->child_pid[i]].utime;
    }
    return get_ticks();
}

long sys_gettimeofday(time_val_t *tv, timezone_t *tz) {
    if (tv) {
        get_utime(tv);
    }
    if (tz) {
        tz->tz_minuteswest = timezone_550W.tz_minuteswest;
        tz->tz_dsttime = timezone_550W.tz_dsttime;
    }
    return 0;
}