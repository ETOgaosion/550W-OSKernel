#pragma once

#include <common/types.h>

typedef struct time_val {
    uint64 sec;  // 自 Unix 纪元起的秒数
    uint64 usec; // 微秒数
} time_val_t;

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern list_head timers;

uint64_t get_timer(void);
uint64_t sys_get_ticks(void);
uint32_t get_wall_time(uint32_t *);

uint64_t sys_get_time_base();

void latency(uint64_t time);
