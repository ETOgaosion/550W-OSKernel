#pragma once

#include <os/pcb.h>

// #define NR_CPUS 2
// void* cpu_stack_pointer[NR_CPUS];
// void* cpu_pcb_pointer[NR_CPUS];
extern spin_lock_t kernel_lock;

void k_smp_init();
void k_smp_wakeup_other_hart();
uint64_t k_smp_get_current_cpu_id();
pcb_t *volatile *k_smp_get_current_running();
pcb_t *k_smp_get_current_pcb();
void k_smp_set_current_pcb(pcb_t *pcb);
void k_smp_sync_current_pcb();
void k_smp_lock_kernel();
void k_smp_unlock_kernel();

long sys_poweroff();