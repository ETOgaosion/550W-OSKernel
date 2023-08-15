#include <asm/csr.h>
#include <asm/pgtable.h>
#include <asm/regs.h>
#include <asm/stack.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/mm.h>
#include <os/smp.h>

extern void user_ret_from_exception();
extern void __global_pointer$();

ptr_t address_base = 0xffffffc080604000lu;

ptr_t get_kernel_address(pid_t pid) {
    return address_base + (pid + 1) * 2 * (4 * PAGE_SIZE);
}

ptr_t get_user_address(pid_t pid) {
    return address_base + pid * 2 * (4 * PAGE_SIZE) + 4 * PAGE_SIZE;
}

void init_context_stack(ptr_t kernel_stack, ptr_t user_stack, int argc, char *argv[], ptr_t entry_point, pcb_t *pcb) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    k_bzero((void *)pt_regs, sizeof(regs_context_t));
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = user_stack;
    pt_regs->regs[reg_ra] = entry_point;
    pt_regs->regs[reg_sp] = pcb->user_sp;
    if (pcb->type == USER_PROCESS) {
        pt_regs->regs[reg_gp] = (reg_t)__global_pointer$;
    } else if (pcb->type == USER_THREAD) {
        regs_context_t *cur_regs = (regs_context_t *)((*current_running)->kernel_sp + sizeof(switchto_context_t));
        pt_regs->regs[reg_gp] = cur_regs->regs[reg_gp];
    }
    pt_regs->regs[reg_tp] = (reg_t)pcb;
    pt_regs->regs[reg_a0] = 0;
    pt_regs->regs[reg_a1] = 0;
    pt_regs->sscratch = (reg_t)pcb;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SUM | SR_SPIE | SR_FS;
    pt_regs->stval = 0;
    pt_regs->scause = 0;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[switch_reg_ra] = (pcb->type == USER_PROCESS || pcb->type == USER_THREAD) ? (reg_t)&user_ret_from_exception : entry_point;
    sw_regs->regs[switch_reg_sp] = pcb->kernel_sp;
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
}

void fork_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    regs_context_t *cur_regs = (regs_context_t *)((*current_running)->kernel_sp + sizeof(switchto_context_t));
    ptr_t cur_address = get_user_address((*current_running)->pid);
    k_memcpy(user_stack - PAGE_SIZE, cur_address - PAGE_SIZE, PAGE_SIZE - 1);
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = (*current_running)->user_sp;

    for (int i = 0; i < NORMAL_REGS_NUM; i++) {
        pt_regs->regs[i] = cur_regs->regs[i];
    }
    // tp
    pt_regs->regs[reg_sp] = pcb->user_sp;
    pt_regs->regs[reg_tp] = (reg_t)pcb;
    // a0
    pt_regs->regs[reg_a0] = 0;
    // pt_regs->regs[8] = user_stack;
    pt_regs->sepc = cur_regs->sepc;
    pt_regs->sstatus = cur_regs->sstatus;
    pt_regs->stval = cur_regs->stval;
    pt_regs->scause = cur_regs->scause;
    pt_regs->sscratch = (reg_t)pcb;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[switch_reg_ra] = (reg_t)user_ret_from_exception;
    // sw_regs->regs[1] = pcb->user_sp;
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
}

void clone_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb, unsigned long flags, void *tls) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    regs_context_t *cur_regs = (regs_context_t *)((*current_running)->kernel_sp + sizeof(switchto_context_t));
    // copy user stack
    ptr_t cur_user_stack = get_user_address((*current_running)->pid);
    ptr_t clone_user_stack = get_user_address(pcb->pid);
    ptr_t off = 4 * 0x1000 - 1;
    char *p = (char *)(clone_user_stack - 1);
    char *q = (char *)(cur_user_stack - 1);
    for (ptr_t i = off + 1; i > 0; i--) {
        *p = *q;
        p--;
        q--;
    }
    pcb->user_sp = user_stack;
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    // copy save context
    for (int i = 0; i < 31; i++) {
        pt_regs->regs[i] = cur_regs->regs[i];
    }
    // tp
    // pt_regs->regs[reg_tp] = (reg_t)pcb;
    pt_regs->regs[reg_sp] = pcb->user_sp;
    // a0
    pt_regs->regs[reg_a0] = 0;
    pt_regs->sepc = cur_regs->sepc;

    pt_regs->sstatus = cur_regs->sstatus;
    pt_regs->stval = cur_regs->stval;
    pt_regs->scause = cur_regs->scause;
    pt_regs->sscratch = (reg_t)pcb;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[switch_reg_ra] = (reg_t)&user_ret_from_exception;
    sw_regs->regs[switch_reg_sp] = pcb->kernel_sp;
    pcb->save_context = pt_regs;
    pcb->switch_context = sw_regs;
    // pcb->save_context->regs[switch_reg_s0] = user_stack == USER_STACK_ADDR ? (*current_running)->save_context->regs[2] : user_stack;
    if (flags & CLONE_SETTLS) {
        pcb->save_context->regs[reg_tp] = (reg_t)tls;
    }
}