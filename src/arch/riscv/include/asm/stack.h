#pragma once

#include <common/type.h>
#include <os/sched.h>

ptr_t get_kernel_address(pid_t pid);

void init_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, pcb_t *pcb, uint64_t argc, char **argv);

void fork_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb);