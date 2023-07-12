#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/privileged.h>
#include <asm/sbi.h>
#include <asm/syscall.h>
#include <common/types.h>
#include <drivers/plic/plic.h>
#include <drivers/virtio/virtio.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <os/irq.h>
#include <os/smp.h>
#include <os/syscall.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
handler_t irq_ext_table[PLIC_NR_IRQS];
uintptr_t riscv_dtb;

long sys_undefined_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause);

void k_init_syscall(void) {
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        syscall[i] = (long (*)()) & sys_undefined_syscall; // only print register info
    }
    // FS
    syscall[SYS_ioctl] = (long (*)())sys_ioctl;
    syscall[SYS_pselect6] = (long (*)())sys_pselect6;
    syscall[SYS_ppoll] = (long (*)())sys_ppoll;
    syscall[SYS_setxattr] = (long (*)())sys_setxattr;
    syscall[SYS_lsetxattr] = (long (*)())sys_lsetxattr;
    syscall[SYS_removexattr] = (long (*)())sys_removexattr;
    syscall[SYS_lremovexattr] = (long (*)())sys_lremovexattr;
    syscall[SYS_getcwd] = (long (*)())sys_getcwd;
    syscall[SYS_dup] = (long (*)())sys_dup;
    syscall[SYS_dup3] = (long (*)())sys_dup3;
    syscall[SYS_fcntl] = (long (*)())sys_fcntl;
    syscall[SYS_flock] = (long (*)())sys_flock;
    syscall[SYS_mknodat] = (long (*)())sys_mknodat;
    syscall[SYS_mkdirat] = (long (*)())sys_mkdirat;
    syscall[SYS_unlinkat] = (long (*)())sys_unlinkat;
    syscall[SYS_symlinkat] = (long (*)())sys_symlinkat;
    syscall[SYS_linkat] = (long (*)())sys_linkat;
    syscall[SYS_umount2] = (long (*)())sys_umount2;
    syscall[SYS_mount] = (long (*)())sys_mount;
    syscall[SYS_pivot_root] = (long (*)())sys_pivot_root;
    syscall[SYS_statfs] = (long (*)())sys_statfs;
    syscall[SYS_ftruncate] = (long (*)())sys_ftruncate;
    syscall[SYS_fallocate] = (long (*)())sys_fallocate;
    syscall[SYS_faccessat] = (long (*)())sys_faccessat;
    syscall[SYS_chdir] = (long (*)())sys_chdir;
    syscall[SYS_fchdir] = (long (*)())sys_fchdir;
    syscall[SYS_chroot] = (long (*)())sys_chroot;
    syscall[SYS_fchmod] = (long (*)())sys_fchmod;
    syscall[SYS_fchmodat] = (long (*)())sys_fchmodat;
    syscall[SYS_fchownat] = (long (*)())sys_fchownat;
    syscall[SYS_fchown] = (long (*)())sys_fchown;
    syscall[SYS_openat] = (long (*)())sys_openat;
    syscall[SYS_close] = (long (*)())sys_close;
    syscall[SYS_pipe2] = (long (*)())sys_pipe2;
    syscall[SYS_getdents64] = (long (*)())sys_getdents64;
    syscall[SYS_lseek] = (long (*)())sys_lseek;
    syscall[SYS_read] = (long (*)())sys_read;
    syscall[SYS_write] = (long (*)())sys_write;
    syscall[SYS_readv] = (long (*)())sys_readv;
    syscall[SYS_writev] = (long (*)())sys_writev;
    syscall[SYS_sendfile] = (long (*)())sys_sendfile64;
    syscall[SYS_readlinkat] = (long (*)())sys_readlinkat;
#if __BITS_PER_LONG == 64
    syscall[SYS_fstatat] = (long (*)())sys_newfstatat;
    syscall[SYS_fstat] = (long (*)())sys_newfstat;
#else
    syscall[SYS_fstatat] = (long (*)())sys_fstatat64;
    syscall[SYS_fstat] = (long (*)())sys_fstat64;
#endif
    syscall[SYS_sync] = (long (*)())sys_sync;
    syscall[SYS_fsync] = (long (*)())sys_fsync;
    syscall[SYS_umask] = (long (*)())sys_umask;
    syscall[SYS_syncfs] = (long (*)())sys_syncfs;
    syscall[SYS_renameat2] = (long (*)())sys_renameat2;

    // System
    syscall[SYS_uname] = (long (*)())sys_uname;
    syscall[SYS_sethostname] = (long (*)())sys_sethostname;
    syscall[SYS_utimensat] = (long (*)())sys_utimensat;
    syscall[SYS_nanosleep] = (long (*)())sys_nanosleep;
    syscall[SYS_setitimer] = (long (*)())sys_setitimer;
    syscall[SYS_clock_settime] = (long (*)())sys_clock_settime;
    syscall[SYS_clock_gettime] = (long (*)())sys_clock_gettime;
    syscall[SYS_clock_getres] = (long (*)())sys_clock_getres;
    syscall[SYS_times] = (long (*)())sys_times;
    syscall[SYS_gettimeofday] = (long (*)())sys_gettimeofday;
    syscall[SYS_adjtimex] = (long (*)())sys_adjtimex;
    syscall[SYS_clock_adjtime] = (long (*)())sys_clock_adjtime;
    syscall[SYS_syslog] = (long (*)())sys_syslog;
    syscall[SYS_reboot] = (long (*)())sys_reboot;
    syscall[SYS_sysinfo] = (long (*)())sys_sysinfo;
    syscall[SYS_msgget] = (long (*)())sys_msgget;
    syscall[SYS_msgctl] = (long (*)())sys_msgctl;
    syscall[SYS_semget] = (long (*)())sys_semget;
    syscall[SYS_semctl] = (long (*)())sys_semctl;
    syscall[SYS_semop] = (long (*)())sys_semop;
    syscall[SYS_shmget] = (long (*)())sys_shmget;
    syscall[SYS_shmctl] = (long (*)())sys_shmctl;
    syscall[SYS_shmat] = (long (*)())sys_shmat;
    syscall[SYS_shmdt] = (long (*)())sys_shmdt;

    // PCB
    syscall[SYS_acct] = (long (*)())sys_acct;
    syscall[SYS_personality] = (long (*)())sys_personality;
    syscall[SYS_exit] = (long (*)())sys_exit;
    syscall[SYS_unshare] = (long (*)())sys_unshare;
    syscall[SYS_kill] = (long (*)())sys_kill;
    syscall[SYS_clone] = (long (*)())sys_clone;
    syscall[SYS_execve] = (long (*)())sys_execve;
    syscall[SYS_wait4] = (long (*)())sys_wait4;
    syscall[SYS_capget] = (long (*)())sys_capget;
    syscall[SYS_capset] = (long (*)())sys_capset;
    syscall[SYS_exit_group] = (long (*)())sys_exit_group;
    syscall[SYS_set_tid_address] = (long (*)())sys_set_tid_address;
    syscall[SYS_tkill] = (long (*)())sys_tkill;
    syscall[SYS_tgkill] = (long (*)())sys_tgkill;
    syscall[SYS_prctl] = (long (*)())sys_prctl;
    syscall[SYS_futex] = (long (*)())sys_futex;
    syscall[SYS_sched_setparam] = (long (*)())sys_sched_setparam;
    syscall[SYS_sched_setscheduler] = (long (*)())sys_sched_setscheduler;
    syscall[SYS_sched_getscheduler] = (long (*)())sys_sched_getscheduler;
    syscall[SYS_sched_getparam] = (long (*)())sys_sched_getparam;
    syscall[SYS_sched_setaffinity] = (long (*)())sys_sched_setaffinity;
    syscall[SYS_sched_getaffinity] = (long (*)())sys_sched_getaffinity;
    syscall[SYS_sched_yield] = (long (*)())sys_sched_yield;
    syscall[SYS_sched_get_priority_max] = (long (*)())sys_sched_get_priority_max;
    syscall[SYS_sched_get_priority_min] = (long (*)())sys_sched_get_priority_min;
    syscall[SYS_rt_sigaction] = (long (*)())sys_rt_sigaction;
    syscall[SYS_rt_sigprocmask] = (long (*)())sys_rt_sigprocmask;
    syscall[SYS_setgid] = (long (*)())sys_setgid;
    syscall[SYS_setuid] = (long (*)())sys_setuid;
    syscall[SYS_getresuid] = (long (*)())sys_getresuid;
    syscall[SYS_getresgid] = (long (*)())sys_getresgid;
    syscall[SYS_setpgid] = (long (*)())sys_setpgid;
    syscall[SYS_getpgid] = (long (*)())sys_getpgid;
    syscall[SYS_getsid] = (long (*)())sys_getsid;
    syscall[SYS_setsid] = (long (*)())sys_setsid;
    syscall[SYS_getgroups] = (long (*)())sys_getgroups;
    syscall[SYS_setgroups] = (long (*)())sys_setgroups;
    syscall[SYS_getpid] = (long (*)())sys_getpid;
    syscall[SYS_getppid] = (long (*)())sys_getppid;
    syscall[SYS_getuid] = (long (*)())sys_getuid;
    syscall[SYS_geteuid] = (long (*)())sys_geteuid;
    syscall[SYS_getgid] = (long (*)())sys_getgid;
    syscall[SYS_getegid] = (long (*)())sys_getegid;
    syscall[SYS_gettid] = (long (*)())sys_gettid;
    syscall[SYS_getrlimit] = (long (*)())sys_getrlimit;
    syscall[SYS_setrlimit] = (long (*)())sys_setrlimit;
    syscall[SYS_getrusage] = (long (*)())sys_getrusage;
    syscall[SYS_prlimit64] = (long (*)())sys_prlimit64;

    // Socket
    syscall[SYS_socket] = (long (*)())sys_socket;
    syscall[SYS_socketpair] = (long (*)())sys_socketpair;
    syscall[SYS_bind] = (long (*)())sys_bind;
    syscall[SYS_listen] = (long (*)())sys_listen;
    syscall[SYS_connect] = (long (*)())sys_connect;
    syscall[SYS_getsockname] = (long (*)())sys_getsockname;
    syscall[SYS_getpeername] = (long (*)())sys_getpeername;
    syscall[SYS_sendto] = (long (*)())sys_sendto;
    syscall[SYS_recvfrom] = (long (*)())sys_recvfrom;
    syscall[SYS_setsockopt] = (long (*)())sys_setsockopt;
    syscall[SYS_getsockopt] = (long (*)())sys_getsockopt;
    syscall[SYS_shutdown] = (long (*)())sys_shutdown;
    syscall[SYS_sendmsg] = (long (*)())sys_sendmsg;
    syscall[SYS_recvmsg] = (long (*)())sys_recvmsg;
    syscall[SYS_readahead] = (long (*)())sys_readahead;

    // mm
    syscall[SYS_brk] = (long (*)())sys_brk;
    syscall[SYS_munmap] = (long (*)())sys_munmap;
    syscall[SYS_mremap] = (long (*)())sys_mremap;
    syscall[SYS_mmap] = (long (*)())sys_mmap;
    syscall[SYS_swapon] = (long (*)())sys_swapon;
    syscall[SYS_swapoff] = (long (*)())sys_swapoff;
    syscall[SYS_mprotect] = (long (*)())sys_mprotect;
    syscall[SYS_msync] = (long (*)())sys_msync;
    syscall[SYS_mlock] = (long (*)())sys_mlock;
    syscall[SYS_munlock] = (long (*)())sys_munlock;
    syscall[SYS_madvise] = (long (*)())sys_madvise;

    // Self Defined
    syscall[SYS_move_cursor] = (long (*)())sys_screen_move_cursor;
    syscall[SYS_screen_clear] = (long (*)())sys_screen_clear;
}

void reset_irq_timer() {
    // note: use sbi_set_timer
    // remember to reschedule
    sbi_set_timer(k_time_get_ticks() + TICKS_INTERVAL);
    k_pcb_scheduler();
}

void user_interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    // call corresponding handler by the value of `cause`
    k_smp_lock_kernel();
    time_val_t now;
    k_time_get_utime(&now);
    time_val_t last_run;
    k_time_minus_utime(&now, &(*current_running)->utime_last, &last_run);
    k_time_add_utime(&last_run, &(*current_running)->resources.ru_utime, &(*current_running)->resources.ru_utime);
    k_time_copy_utime(&now, &(*current_running)->stime_last);
    uint64_t check = cause;
    if (check >> 63) {
        irq_table[cause - ((uint64_t)1 << 63)](regs, stval, cause, cpuid);
    } else {
        exc_table[cause](regs, stval, cause, cpuid);
    }
    k_time_get_utime(&now);
    k_time_copy_utime(&now, &(*current_running)->utime_last);
    k_time_minus_utime(&now, &(*current_running)->stime_last, &last_run);
    k_time_add_utime(&last_run, &(*current_running)->resources.ru_stime, &(*current_running)->resources.ru_stime);
}

spin_lock_t kernel_exception_lock = {.flag = UNLOCKED};

void kernel_interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    k_spin_lock_acquire(&kernel_exception_lock);
    current_running = k_smp_get_current_running();
    uint64_t check = cause;
    if (check >> 63) {
        irq_table[cause - ((uint64_t)1 << 63)](regs, stval, cause, cpuid);
    } else {
        exc_table[cause](regs, stval, cause, cpuid);
    }
    sbi_set_timer(k_time_get_ticks() + TICKS_INTERVAL);
    k_spin_lock_release(&kernel_exception_lock);
}

void handle_int_irq(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    reset_irq_timer();
}

void handle_ext_irq(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    int irq = d_plic_claim();
    if (irq) {
        irq_ext_table[irq](regs, interrupt, cause, cpuid);
        d_plic_complete(irq);
    }
}

void handler_virtio_intr(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    k_spin_lock_acquire(&disk.vdisk_lock);

    // the device won't raise another interrupt until we tell it
    // we've seen this interrupt, which the following line does.
    // this may race with the device writing new entries to
    // the "used" ring, in which case we may process the new
    // completion entries in this interrupt, and have nothing to do
    // in the next interrupt, which is harmless.

    // the device increments disk.used->idx when it
    // adds an entry to the used ring.

    while ((disk.used_idx % DESC_NUM) != (disk.used->id % DESC_NUM)) {
        int id = disk.used->elems[disk.used_idx].id;

        if (disk.info[id].status != 0) {
            panic("d_virtio_disk_intr status");
        }

        disk.info[id].b->disk = 0; // disk is done with buf
        // k_pcb_wakeup(disk.info[id].b);

        disk.used_idx = (disk.used_idx + 1) % DESC_NUM;
    }
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    k_spin_lock_release(&disk.vdisk_lock);
}

PTE *check_pf(uint64_t va, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) {
        return 0;
    }
    PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        return 0;
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    if (pld[vpn0] == 0) {
        return 0;
    }
    return &pld[vpn0];
}

void handle_disk_exc(uint64_t stval, PTE *pte_addr) {
    pcb_t *cur = *current_running;
    uint64_t newmem_addr = k_mm_alloc_mem(1, stval);
    k_mm_map(stval, kva2pa(newmem_addr), (pa2kva(cur->pgdir << 12)));
    k_mm_get_back_disk(stval, newmem_addr);
    (*pte_addr) = (*pte_addr) | 1;
}

void handle_pf_exc(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    pcb_t *cur = *current_running;

    PTE *pte_addr = check_pf(stval, (pa2kva(cur->pgdir << 12)));
    if (pte_addr != 0) {
        PTE va_pte = *pte_addr;
        if (cause == EXCC_STORE_PAGE_FAULT) {
            // child process
            if (cur->father_pid != cur->pid && cur->father_pid != 0) {
                k_mm_fork_page_helper(stval, (pa2kva(cur->pgdir << 12)), (pa2kva(pcb[cur->father_pid].pgdir << 12)));
            } else {
                // father process
                for (int i = 0; i < NUM_MAX_CHILD; i++) {
                    if (cur->child_pids[i] == 0) {
                        continue;
                    }
                    k_mm_fork_page_helper(stval, (pa2kva(pcb[cur->child_pids[i]].pgdir << 12)), (pa2kva(cur->pgdir << 12)));
                }
            }
            *pte_addr = (*pte_addr) | (3 << 6);
            *pte_addr = (*pte_addr) | ((uint64_t)1 << 2);
        } else if (cause == EXCC_INST_PAGE_FAULT) {
            *pte_addr = (*pte_addr) | (3 << 6);
        } else if ((((va_pte >> 6) & 1) == 0) && (cause == EXCC_LOAD_PAGE_FAULT)) {
            *pte_addr = (*pte_addr) | ((uint64_t)1 << 6);
        } else if ((va_pte & 1) == 0) { // physical frame was on disk
            handle_disk_exc(stval, pte_addr);
        }
    } else { // No virtual-physical map
        k_mm_alloc_page_helper(stval, (pa2kva(cur->pgdir << 12)));
        local_flush_tlb_all();
    }
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid);

void k_init_exception() {
    for (int i = 0; i < IRQC_COUNT; i++) {
        irq_table[i] = handle_other;
    }
    irq_table[IRQC_S_TIMER] = handle_int_irq;
    irq_table[IRQC_U_EXT] = handle_ext_irq;
    irq_table[IRQC_S_EXT] = handle_ext_irq;
    for (int i = 0; i < EXCC_COUNT; i++) {
        exc_table[i] = handle_other;
    }
    exc_table[EXCC_SYSCALL] = handle_syscall_exc;
    exc_table[EXCC_LOAD_PAGE_FAULT] = handle_pf_exc;
    exc_table[EXCC_STORE_PAGE_FAULT] = handle_pf_exc;
    exc_table[EXCC_INST_PAGE_FAULT] = handle_pf_exc;
    irq_ext_table[IRQC_EXT_VIRTIO_BLK_IEQ] = handler_virtio_intr;
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    prints("cpuid: %d\n", cpuid);

    char *reg_name[] = {" ra  ", " sp  ", " gp  ", " tp  ", " t0  ", " t1  ", " t2  ", "s0/fp", " s1  ", " a0  ", " a1  ", " a2  ", " a3  ", " a4  ", " a5  ", " a6  ", " a7  ", " s2  ", " s3  ", " s4  ", " s5  ", " s6  ", " s7  ", " s8  ", " s9  ", " s10 ", " s11 ", " t3  ", " t4  ", " t5  ", " t6  "};
    for (int i = 0; i < NORMAL_REGS_NUM; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            prints("%s : %016lx ", reg_name[i + j], regs->regs[i + j]);
        }
        prints("\n\r");
    }
    prints("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r", regs->sstatus, regs->sbadaddr, regs->scause);
    prints("stval: 0x%lx cause: %lx\n\r", stval, cause);
    prints("sepc: 0x%lx\n\r", regs->sepc);
    // k_print("mhartid: 0x%lx\n\r", k_smp_get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    prints("[Backtrace]\n\r");
    prints("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE)
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t *)(fp - 8);
        uintptr_t prev_fp = *(uintptr_t *)(fp - 16);

        prints("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}

long sys_undefined_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause) {
    k_print(">[ERROR] unkonwn syscall, undefined syscall number: %d\n!", regs->regs[17]);
    handle_other(regs, interrupt, cause, k_smp_get_current_cpu_id());
    return -1;
}