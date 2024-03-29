#include <asm/context.h>
#include <asm/regs.h>
#include <asm/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall_exc(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t sepc) {
    if (syscall[regs->regs[reg_a7]] == (long (*)()) & sys_undefined_syscall) {
        sys_undefined_syscall(regs, interrupt, cause, sepc);
    }
    // syscall[fn](arg1, arg2, arg3)
    regs->sepc = regs->sepc + 4;
    regs->regs[reg_a0] = syscall[regs->regs[reg_a7]](regs->regs[reg_a0], regs->regs[reg_a1], regs->regs[reg_a2], regs->regs[reg_a3], regs->regs[reg_a4], regs->regs[reg_a5], regs->regs[reg_a6]);
}
