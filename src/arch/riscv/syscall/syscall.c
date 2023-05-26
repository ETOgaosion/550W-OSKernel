#include <asm/context.h>
#include <asm/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall_exc(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    // syscall[fn](arg1, arg2, arg3)
    regs->sepc = regs->sepc + 4;
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10], regs->regs[11], regs->regs[12], regs->regs[13]);
}
