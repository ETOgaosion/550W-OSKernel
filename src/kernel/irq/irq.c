#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/syscall.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <os/irq.h>
#include <os/smp.h>
#include <os/syscall.h>

#define TICKS_INTERVAL 200

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

void init_syscall(void) {
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        syscall[i] = (long (*)()) & sys_undefined_syscall; // only print register info
    }
    // FS
    // syscall[SYS_getcwd] = (long (*)())sys_getcwd;
    // syscall[SYS_dup] = (long (*)())sys_dup;
    // syscall[SYS_dup3] = (long (*)())sys_dup3;
    // syscall[SYS_mkdirat] = (long (*)())sys_mkdirat;
    // syscall[SYS_unlinkat] = (long (*)())sys_unlinkat;
    // syscall[SYS_linkat] = (long (*)())sys_linkat;
    // syscall[SYS_umount2] = (long (*)())sys_umount2;
    // syscall[SYS_mount] = (long (*)())sys_mount;
    // syscall[SYS_chdir] = (long (*)())sys_chdir;
    // syscall[SYS_openat] = (long (*)())sys_openat;
    // syscall[SYS_close] = (long (*)())sys_close;
    // syscall[SYS_pipe2] = (long (*)())sys_pipe2;
    // syscall[SYS_getdents64] = (long (*)())sys_getdents64;
    // syscall[SYS_read] = (long (*)())sys_read;
    // syscall[SYS_write] = (long (*)())sys_write;
    // syscall[SYS_fstat] = (long (*)())sys_fstat;
    // Terminal
    syscall[SYS_uname] = (long (*)())sys_uname;
    // Functions
    syscall[SYS_nanosleep] = (long (*)())sys_nanosleep;
    syscall[SYS_times] = (long (*)())sys_times;
    syscall[SYS_time] = (long (*)())sys_time;
    syscall[SYS_gettimeofday] = (long (*)())sys_gettimeofday;
    // syscall[SYS_mailread] = (long (*)())sys_mailread;
    // syscall[SYS_mailwrite] = (long (*)())sys_mailwrite;
    // Process & Threads
    syscall[SYS_exit] = (long (*)())sys_exit;
    // syscall[SYS_clone] = (long (*)())sys_clone;
    // syscall[SYS_execve] = (long (*)())sys_execve;
    syscall[SYS_wait4] = (long (*)())sys_wait4;
    syscall[SYS_spawn] = (long (*)())sys_spawn;
    syscall[SYS_sched_yield] = (long (*)())sys_sched_yield;
    syscall[SYS_setpriority] = (long (*)())sys_setpriority;
    syscall[SYS_get_mempolicy] = (long (*)())sys_getpriority;
    syscall[SYS_getpid] = (long (*)())sys_getpid;
    syscall[SYS_getppid] = (long (*)())sys_getppid;
    // Memory
    // syscall[SYS_brk] = (long (*)())sys_brk;
    // syscall[SYS_munmap] = (long (*)())sys_munmap;
    // syscall[SYS_mmap] = (long (*)())sys_mmap;
}

void reset_irq_timer() {
    k_screen_reflush();
    // note: use sbi_set_timer
    // remember to reschedule
    sbi_set_timer(get_ticks() + get_time_base() / TICKS_INTERVAL);
    k_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    lock_kernel();
    // call corresponding handler by the value of `cause`
    (*current_running) = get_current_running();
    long ticks = get_ticks();
    (*current_running)->utime += (ticks - (*current_running)->utime_last);
    (*current_running)->stime_last = ticks;
    uint64_t check = cause;
    // if(check>>63 && check%16!=5)
    //{
    //    screen_move_cursor(1, 1);
    //    prints("cause: %lx\n", cause);
    //}
    if (check >> 63) {
        irq_table[cause - ((uint64_t)1 << 63)](regs, stval, cause, cpuid);
    } else {
        exc_table[cause](regs, stval, cause, cpuid);
    }
    ticks = get_ticks();
    (*current_running)->utime_last = ticks;
    (*current_running)->stime += (ticks - (*current_running)->stime_last);
    unlock_kernel();
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) { reset_irq_timer(); }

PTE *check_pf(uint64_t va, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0)
        return 0;
    PTE *pmd = get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0)
        return 0;
    PTE *pld = get_kva(pmd[vpn1]);
    if (pld[vpn0] == 0)
        return 0;
    return &pld[vpn0];
}

void handle_disk(uint64_t stval, PTE *pte_addr) {
    pcb_t *cur = *current_running;
    allocpid = cur->pid;
    uint64_t newmem_addr = allocmem(1, stval);
    map(stval, kva2pa(newmem_addr), (pa2kva(cur->pgdir << 12)));
    getbackdisk(stval, newmem_addr);
    (*pte_addr) = (*pte_addr) | 1;
}

void handle_pf(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    pcb_t *cur = *current_running;

    PTE *pte_addr = check_pf(stval, (pa2kva(cur->pgdir << 12)));
    if (pte_addr != 0) {
        PTE va_pte = *pte_addr;
        if (cause == EXCC_STORE_PAGE_FAULT) {
            // child process
            allocpid = cur->pid;
            if (cur->father_pid != cur->pid) {
                fork_page_helper(stval, (pa2kva(cur->pgdir << 12)), (pa2kva(pcb[cur->father_pid].pgdir << 12)));
            } else {
                // father process
                for (int i = 0; i < cur->child_num; i++) {
                    fork_page_helper(stval, (pa2kva(pcb[cur->child_pid[i]].pgdir << 12)), (pa2kva(cur->pgdir << 12)));
                }
            }
            *pte_addr = (*pte_addr) | (3 << 6);
            *pte_addr = (*pte_addr) | ((uint64_t)1 << 2);
        } else if (cause == EXCC_INST_PAGE_FAULT)
            *pte_addr = (*pte_addr) | (3 << 6);
        else if ((((va_pte >> 6) & 1) == 0) && (cause == EXCC_LOAD_PAGE_FAULT))
            *pte_addr = (*pte_addr) | ((uint64_t)1 << 6);
        else if ((va_pte & 1) == 0) // physical frame was on disk
            handle_disk(stval, pte_addr);
    } else { // No virtual-physical map
        allocpid = cur->pid;
        alloc_page_helper(stval, (pa2kva(cur->pgdir << 12)));
        local_flush_tlb_all();
    }
}

void init_exception() {
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for (int i = 0; i < IRQC_COUNT; i++) {
        irq_table[i] = handle_other;
    }
    irq_table[IRQC_S_TIMER] = handle_int;
    for (int i = 0; i < EXCC_COUNT; i++) {
        exc_table[i] = handle_other;
    }
    exc_table[EXCC_SYSCALL] = handle_syscall;
    exc_table[EXCC_LOAD_PAGE_FAULT] = handle_pf;
    exc_table[EXCC_STORE_PAGE_FAULT] = handle_pf;
    exc_table[EXCC_INST_PAGE_FAULT] = handle_pf;
    // setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    prints("cpuid: %d\n", cpuid);

    char *reg_name[] = {"zero ", " ra  ", " sp  ", " gp  ", " tp  ", " t0  ", " t1  ", " t2  ", "s0/fp", " s1  ", " a0  ", " a1  ", " a2  ", " a3  ", " a4  ", " a5  ", " a6  ", " a7  ", " s2  ", " s3  ", " s4  ", " s5  ", " s6  ", " s7  ", " s8  ", " s9  ", " s10 ", " s11 ", " t3  ", " t4  ", " t5  ", " t6  "};
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            prints("%s : %016lx ", reg_name[i + j], regs->regs[i + j]);
        }
        prints("\n\r");
    }
    prints("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r", regs->sstatus, regs->sbadaddr, regs->scause);
    prints("stval: 0x%lx cause: %lx\n\r", stval, cause);
    prints("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    prints("[Backtrace]\n\r");
    prints("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t *)(fp - 8);
        uintptr_t prev_fp = *(uintptr_t *)(fp - 16);

        prints("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}

long sys_undefined_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause) {
    printk(">[ERROR] unkonwn syscall, undefined syscall number: %d\n!", regs->regs[17]);
    handle_other(regs, interrupt, cause, get_current_cpu_id());
    return -1;
}