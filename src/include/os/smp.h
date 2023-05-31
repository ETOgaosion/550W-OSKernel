#pragma once

#include <os/pcb.h>

// #define NR_CPUS 2
// void* cpu_stack_pointer[NR_CPUS];
// void* cpu_pcb_pointer[NR_CPUS];
void k_smp_init();
void k_wakeup_other_hart();
uint64_t get_current_cpu_id();
pcb_t *volatile *k_get_current_running();
void k_lock_kernel();
void k_unlock_kernel();
