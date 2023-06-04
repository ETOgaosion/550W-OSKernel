#pragma once

#include <common/types.h>
#include <os/pcb.h>

ptr_t get_kernel_address(pid_t pid);

void init_context_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, pcb_t *pcb);

void fork_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb);

void clone_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb, unsigned long flags, void *tls, int (*fn)(void *arg), void* arg);