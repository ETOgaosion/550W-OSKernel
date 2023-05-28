#include <asm/csr.h>
#include <asm/pgtable.h>
#include <asm/stack.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/mm.h>
#include <os/smp.h>

extern void ret_from_exception();
extern void __global_pointer$();

ptr_t address_base = 0xffffffc080504000lu;

ptr_t get_kernel_address(pid_t pid) {
    return address_base + (pid + 1) * 2 * PAGE_SIZE;
}

ptr_t get_user_address(pid_t pid) {
    return address_base + pid * 2 * PAGE_SIZE + PAGE_SIZE;
}

void init_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, pcb_t *pcb) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    k_memset(pt_regs, 0, sizeof(regs_context_t));
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = user_stack;
    pt_regs->regs[0] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    if (pcb->type == USER_PROCESS) {
        pt_regs->regs[3] = (reg_t)__global_pointer$;
    } else if (pcb->type == USER_THREAD) {
        regs_context_t *cur_regs = (regs_context_t *)((*current_running)->kernel_sp + sizeof(switchto_context_t));
        pt_regs->regs[3] = cur_regs->regs[3];
    }
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SUM | SR_FS;
    pt_regs->sbadaddr = 0;
    pt_regs->scause = 0;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[0] = (pcb->type == USER_PROCESS || pcb->type == USER_THREAD) ? (reg_t)&ret_from_exception : entry_point;
    sw_regs->regs[1] = pcb->kernel_sp;
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
}

void fork_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    regs_context_t *cur_regs = (regs_context_t *)((*current_running)->kernel_sp + sizeof(switchto_context_t));
    ptr_t cur_address = get_user_address((*current_running)->pid);
    k_memcpy((uint8_t *)(user_stack - PAGE_SIZE), (uint8_t *)(cur_address - PAGE_SIZE), PAGE_SIZE - 1);
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = (*current_running)->user_sp;

    for (int i = 0; i < 32; i++) {
        pt_regs->regs[i] = cur_regs->regs[i];
    }
    // tp
    pt_regs->regs[2] = pcb->user_sp;
    pt_regs->regs[4] = (reg_t)pcb;
    // a0
    pt_regs->regs[10] = 0;
    // pt_regs->regs[8] = user_stack;
    pt_regs->sepc = cur_regs->sepc;
    pt_regs->sstatus = cur_regs->sstatus;
    pt_regs->sbadaddr = cur_regs->sbadaddr;
    pt_regs->scause = cur_regs->scause;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[0] = (reg_t)ret_from_exception;
    // sw_regs->regs[1] = pcb->user_sp;
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
}

void clone_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb, unsigned long flags, void *tls) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
    pcb->kernel_sp = (uintptr_t)pcb->switch_context;
    pcb->switch_context->regs[0] = (reg_t)&ret_from_exception;
    pcb->switch_context->regs[1] = pcb->kernel_sp;
    pcb->save_context->regs[2] = user_stack == USER_STACK_ADDR ? (*current_running)->save_context->regs[2] : user_stack;
    if (flags & CLONE_SETTLS) {
        pcb->save_context->regs[4] = (reg_t)tls;
    }
    pcb->save_context->regs[10] = 0;
}