#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/syscall.h>
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
    syscall[SYS_read] = (long (*)())sys_read;
    syscall[SYS_write] = (long (*)())sys_write;
    // syscall[SYS_fstat] = (long (*)())sys_fstat;
    // syscall[SYS_munmap] = (long (*)())sys_munmap;
    // syscall[SYS_mremap] = (long (*)())sys_mremap;
    // syscall[SYS_mmap] = (long (*)())sys_mmap;
    // Terminal
    syscall[SYS_uname] = (long (*)())sys_uname;
    // Functions
    syscall[SYS_nanosleep] = (long (*)())sys_nanosleep;
    syscall[SYS_times] = (long (*)())sys_times;
    syscall[SYS_time] = (long (*)())sys_time;
    syscall[SYS_gettimeofday] = (long (*)())sys_gettimeofday;
    syscall[SYS_mailread] = (long (*)())sys_mailread;
    syscall[SYS_mailwrite] = (long (*)())sys_mailwrite;
    // Process & Threads
    syscall[SYS_exit] = (long (*)())sys_exit;
    syscall[SYS_clone] = (long (*)())sys_clone;
    syscall[SYS_execve] = (long (*)())sys_execve;
    syscall[SYS_wait4] = (long (*)())sys_wait4;
    syscall[SYS_spawn] = (long (*)())sys_spawn;
    syscall[SYS_sched_yield] = (long (*)())sys_sched_yield;
    syscall[SYS_setpriority] = (long (*)())sys_setpriority;
    syscall[SYS_get_mempolicy] = (long (*)())sys_getpriority;
    syscall[SYS_getpid] = (long (*)())sys_getpid;
    syscall[SYS_getppid] = (long (*)())sys_getppid;
    syscall[SYS_sched_setaffinity] = (long (*)())sys_sched_setaffinity;

    // Memory
    syscall[SYS_brk] = (long (*)())sys_brk;

    // Self Defined
    syscall[SYS_move_cursor] = (long (*)())sys_screen_move_cursor;
    syscall[SYS_screen_clear] = (long (*)())sys_screen_clear;

    syscall[SYS_SD_READ] = (long (*)())sys_sd_read;
    syscall[SYS_SD_WRITE] = (long (*)())sys_sd_write;
    syscall[SYS_SD_WRITE] = (long (*)())sys_sd_release;
}

void reset_irq_timer() {
    // note: use sbi_set_timer
    // remember to reschedule
    sbi_set_timer(get_ticks() + TICKS_INTERVAL);
    k_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    // call corresponding handler by the value of `cause`
    k_lock_kernel();
    current_running = k_get_current_running();
    time_val_t now;
    get_utime(&now);
    time_val_t last_run;
    minus_utime(&now, &(*current_running)->utime_last, &last_run);
    add_utime(&last_run, &(*current_running)->resources.ru_utime, &(*current_running)->resources.ru_utime);
    copy_utime(&now, &(*current_running)->stime_last);
    uint64_t check = cause;
    if (check >> 63) {
        irq_table[cause - ((uint64_t)1 << 63)](regs, stval, cause, cpuid);
    } else {
        exc_table[cause](regs, stval, cause, cpuid);
    }
    get_utime(&now);
    copy_utime(&now, &(*current_running)->utime_last);
    minus_utime(&now, &(*current_running)->stime_last, &last_run);
    add_utime(&last_run, &(*current_running)->resources.ru_stime, &(*current_running)->resources.ru_stime);
}

void handle_int_irq(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    reset_irq_timer();
}

void handle_ext_irq(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    int irq = plic_claim();
    irq_ext_table[irq](regs, interrupt, cause, cpuid);
    plic_complete(irq);
}

void handler_virtio_intr(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    k_spin_lock_acquire(&disk.vdisk_lock);

    // the device won't raise another interrupt until we tell it
    // we've seen this interrupt, which the following line does.
    // this may race with the device writing new entries to
    // the "used" ring, in which case we may process the new
    // completion entries in this interrupt, and have nothing to do
    // in the next interrupt, which is harmless.
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    // the device increments disk.used->idx when it
    // adds an entry to the used ring.

    while (disk.used_idx != disk.used->idx) {
        int id = disk.used->ring[disk.used_idx % DESC_NUM].id;

        if (disk.info[id].status != 0) {
            panic("virtio_disk_intr status");
        }

        buf_t *b = disk.info[id].b;
        b->disk = 0; // disk is done with buf
        k_wakeup(b);

        disk.used_idx = (disk.used_idx + 1) % DESC_NUM;
    }

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
    PTE *pmd = get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        return 0;
    }
    PTE *pld = get_kva(pmd[vpn1]);
    if (pld[vpn0] == 0) {
        return 0;
    }
    return &pld[vpn0];
}

void handle_disk_exc(uint64_t stval, PTE *pte_addr) {
    pcb_t *cur = *current_running;
    uint64_t newmem_addr = k_alloc_mem(1, stval);
    map(stval, kva2pa(newmem_addr), (pa2kva(cur->pgdir << 12)));
    k_get_back_disk(stval, newmem_addr);
    (*pte_addr) = (*pte_addr) | 1;
}

void handle_pf_exc(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    pcb_t *cur = *current_running;

    PTE *pte_addr = check_pf(stval, (pa2kva(cur->pgdir << 12)));
    if (pte_addr != 0) {
        PTE va_pte = *pte_addr;
        if (cause == EXCC_STORE_PAGE_FAULT) {
            // child process
            if (cur->father_pid != cur->pid) {
                fork_page_helper(stval, (pa2kva(cur->pgdir << 12)), (pa2kva(pcb[cur->father_pid].pgdir << 12)));
            } else {
                // father process
                for (int i = 0; i < cur->child_num; i++) {
                    fork_page_helper(stval, (pa2kva(pcb[cur->child_pids[i]].pgdir << 12)), (pa2kva(cur->pgdir << 12)));
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
        k_alloc_page_helper(stval, (pa2kva(cur->pgdir << 12)));
        local_flush_tlb_all();
    }
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid);

void init_exception() {
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