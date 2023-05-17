#pragma once

#include <common/type.h>

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern list_head timers;

uint64_t get_timer(void);
uint64_t sys_get_ticks(void);
uint32_t get_wall_time(uint32_t *);

uint64_t sys_get_time_base();

void latency(uint64_t time);
