#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/syscall.h>
#include <drivers/screen.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/syscall.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

void init_syscall(void) {
    syscall[SYSCALL_SLEEP] = (long (*)())sys_sleep;
    syscall[SYSCALL_YIELD] = (long (*)())sys_scheduler;
    syscall[SYSCALL_READ] = (long (*)())sys_port_read;
    syscall[SYSCALL_WRITE] = (long (*)())sys_screen_write;
    syscall[SYSCALL_FORK] = (long (*)())sys_fork;
    syscall[SYSCALL_CURSOR] = (long (*)())sys_screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (long (*)())sys_screen_reflush;
    syscall[SYSCALL_GET_TICK] = (long (*)())sys_get_ticks;
    syscall[SYSCALL_GET_TIMEBASE] = (long (*)())sys_get_time_base;
    syscall[SYSCALL_LOCK_INIT] = (long (*)())sys_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQUIRE] = (long (*)())sys_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE] = (long (*)())sys_mutex_lock_release;
    syscall[SYSCALL_GET_WALL_TIME] = (long (*)())get_wall_time;
    syscall[SYSCALL_CLEAR] = (long (*)())sys_screen_clear;
    syscall[SYSCALL_PS] = (long (*)())sys_process_show;
    syscall[SYSCALL_SPAWN] = (long (*)())sys_spawn;
    syscall[SYSCALL_KILL] = (long (*)())sys_kill;
    syscall[SYSCALL_EXIT] = (long (*)())sys_exit;
    syscall[SYSCALL_WAITPID] = (long (*)())sys_waitpid;
    syscall[SYSCALL_SEMAPHORE_INIT] = (long (*)())sys_semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP] = (long (*)())sys_semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN] = (long (*)())sys_semaphore_down;
    syscall[SYSCALL_SEMAPHORE_DESTROY] = (long (*)())sys_semaphore_destroy;
    syscall[SYSCALL_BARRIER_INIT] = (long (*)())sys_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = (long (*)())sys_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY] = (long (*)())sys_barrier_destroy;
    syscall[SYSCALL_MBOX_OPEN] = (long (*)())sys_mbox_open;
    syscall[SYSCALL_MBOX_SEND] = (long (*)())sys_mbox_send;
    syscall[SYSCALL_MBOX_CLOSE] = (long (*)())sys_mbox_close;
    syscall[SYSCALL_MBOX_RECV] = (long (*)())sys_mbox_recv;
    syscall[SYSCALL_TASKSET] = (long (*)())sys_taskset;
    syscall[SYSCALL_TEST_RECV] = (long (*)())sys_test_recv;
    syscall[SYSCALL_TEST_SEND] = (long (*)())sys_test_send;
    syscall[SYSCALL_EXEC] = (long (*)())sys_exec;
    syscall[SYSCALL_MTHREAD_CREATE] = (long (*)())sys_mthread_create;
    syscall[SYSCALL_GETPA] = (long (*)())sys_getpa;
    syscall[SYSCALL_SHMPAGE_GET] = (long (*)())sys_shm_page_get;
    syscall[SYSCALL_SHMPAGE_DT] = (long (*)())sys_shm_page_dt;
    syscall[SYSCALL_MKFS] = (long (*)())sys_mkfs;
    syscall[SYSCALL_STATFS] = (long (*)())sys_statfs;
    syscall[SYSCALL_CD] = (long (*)())sys_cd;
    syscall[SYSCALL_MKDIR] = (long (*)())sys_mkdir;
    syscall[SYSCALL_RMDIR] = (long (*)())sys_rmdir;
    syscall[SYSCALL_LS] = (long (*)())sys_ls;
    syscall[SYSCALL_TOUCH] = (long (*)())sys_touch;
    syscall[SYSCALL_CAT] = (long (*)())sys_cat;
    syscall[SYSCALL_FOPEN] = (long (*)())sys_fopen;
    syscall[SYSCALL_FREAD] = (long (*)())sys_fread;
    syscall[SYSCALL_FWRITE] = (long (*)())sys_fwrite;
    syscall[SYSCALL_FCLOSE] = (long (*)())sys_fclose;
    syscall[SYSCALL_LN] = (long (*)())sys_ln;
    syscall[SYSCALL_RM] = (long (*)())sys_rm;
    syscall[SYSCALL_LSEEK] = (long (*)())sys_lseek;
    // initialize system call table.
}

void reset_irq_timer() {
    uint64_t each = time_base / 8000;
    uint64_t delta = time_base / 500;
    int prior = pcb[current_running->pid].priority;
    uint64_t next = each + delta * prior;
    sbi_set_timer(sys_get_ticks() + next);
    // note: use sbi_set_timer
    // remember to reschedule
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    // call corresponding handler by the value of `cause`
    while (atomic_swap(1, (ptr_t)&cpu_lock) != 0)
        ;
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
    atomic_swap(0, (ptr_t)&cpu_lock);
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid) {
    check_sleeping();
    sys_scheduler();
    reset_irq_timer();
}

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

void handle_disk(uint64_t stval, PTE *pte_addr, int cpuid) {
    pcb_t *cur;
    if (cpuid == 0)
        cur = current_running0;
    else /*if (cpuid == 1) */
        cur = current_running1;
    allocpid = cur->pid;
    uint64_t newmem_addr = allocmem(1, stval);
    map(stval, kva2pa(newmem_addr), (pa2kva(cur->pgdir << 12)));
    getbackdisk(stval, newmem_addr);
    (*pte_addr) = (*pte_addr) | 1;
}

void handle_pf(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid) {
    pcb_t *cur;
    if (cpuid == 0)
        cur = current_running0;
    else /*if (cpuid == 1) */
        cur = current_running1;

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
            handle_disk(stval, pte_addr, cpuid);
    } else { // No virtual-physical map
        allocpid = cur->pid;
        alloc_page_helper(stval, (pa2kva(cur->pgdir << 12)));
        local_flush_tlb_all();
    }
}

void handle_irq(regs_context_t *regs, int irq) {}

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
