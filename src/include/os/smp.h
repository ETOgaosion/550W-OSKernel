#pragma once

#include <os/pcb.h>

// #define NR_CPUS 2
// void* cpu_stack_pointer[NR_CPUS];
// void* cpu_pcb_pointer[NR_CPUS];
void smp_init();
void wakeup_other_hart();
uint64_t get_current_cpu_id();
pcb_t *volatile *get_current_running();
void lock_kernel();
void unlock_kernel();
