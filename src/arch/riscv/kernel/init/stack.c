#include <asm/pgtable.h>
#include <asm/stack.h>
#include <lib/stdio.h>
#include <os/smp.h>

extern void ret_from_exception();
extern void __global_pointer$();

ptr_t address_base = 0xffffffc050504000lu;

ptr_t get_kernel_address(pid_t pid) { return address_base + (pid + 1) * 0x2000; }

ptr_t get_user_address(pid_t pid) { return address_base + pid * 0x2000 + 0x1000; }

void init_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, pcb_t *pcb, uint64_t argc, char **argv) {
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    uint64_t user_stack_kva = kernel_stack - 0x1000;

    for (int i = 5; i < 32; i++)
        pt_regs->regs[i] = 0;
    if (argv != NULL) {
        char **u_argv = (char **)(user_stack_kva - 100);
        for (int i = 0; i < argc; i++) {
            char *s = (char *)(user_stack_kva - 100 - 20 * (i + 1));
            char *t = argv[i];
            int j = 0;
            for (j = 0; t[j] != '\0'; j++)
                s[j] = t[j];
            s[j] = '\0';
            u_argv[i] = (char *)(user_stack - 100 - 20 * (i + 1));
            // prints("%x\n",u_argv[i]);
        }
        pcb->user_sp = user_stack - 256;
        // prints("sp:%x\n", pcb->user_sp);
        pt_regs->regs[11] = user_stack - 100;
    } else
        pcb->user_sp = user_stack;
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pt_regs->regs[0] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    if (pcb->type == USER_PROCESS)
        pt_regs->regs[3] = (reg_t)__global_pointer$;
    else if (pcb->type == USER_THREAD) {
        regs_context_t *cur_regs = (regs_context_t *)(current_running->kernel_sp + sizeof(switchto_context_t));
        pt_regs->regs[3] = cur_regs->regs[3];
    }
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->regs[10] = argc;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0x40020;
    pt_regs->sbadaddr = 0;
    pt_regs->scause = 0;

    switchto_context_t *sw_regs = (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[0] = (reg_t)ret_from_exception;
    // sw_regs->regs[1] = pcb->user_sp;
}

void fork_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    regs_context_t *cur_regs = (regs_context_t *)(current_running->kernel_sp + sizeof(switchto_context_t));
    ptr_t cur_address = get_user_address(current_running->pid);
    ptr_t off = 0x1000 - 1;
    char *p = (char *)(user_stack - 1);
    char *q = (char *)(cur_address - 1);
    // i is unsigned
    // printk("here1\n");
    for (ptr_t i = off + 1; i > 0; i--) {
        *p = *q;
        p--;
        q--;
    }
    pcb->user_sp = current_running->user_sp;
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    for (int i = 0; i < 32; i++)
        pt_regs->regs[i] = cur_regs->regs[i];
    // tp
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->regs[2] = pcb->user_sp;
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
}