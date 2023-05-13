#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <atomic.h>
#include <os/mm.h>
#include <pgtable.h>

#include <emacps/xemacps_example.h>
#include <plic.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    uint64_t each = time_base / 8000;
    uint64_t delta = time_base / 500;
    int prior = pcb[current_running->pid].priority;
    uint64_t next = each + delta * prior;
    sbi_set_timer(get_ticks()+next);
    // note: use sbi_set_timer
    // remember to reschedule
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    while(atomic_swap(1,&cpu_lock)!=0)
        ;
    uint64_t check = cause;
    //if(check>>63 && check%16!=5)
    //{
    //    screen_move_cursor(1, 1);
    //    prints("cause: %lx\n", cause);
    //}
    if(check>>63)
    {
        irq_table[cause-(1<<63)](regs, stval, cause, cpuid);
    }else{
        exc_table[cause](regs, stval, cause, cpuid);
    }
    atomic_swap(0, &cpu_lock);
}

extern int net_poll_mode;
void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause, uint64_t cpuid)
{
    check_sleeping();
    if(net_poll_mode==2)
    {
        check_send();
        check_recv();
    }
    do_scheduler();
    reset_irq_timer();
}

PTE* check_pf(uint64_t va, PTE* pgdir)
{
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) 
        return 0;
    PTE *pmd = (PTE *)get_kva(pgdir[vpn2]);
    if(pmd[vpn1] == 0)
        return 0;
    PTE *pld = (PTE*)get_kva(pmd[vpn1]);
    if(pld[vpn0] == 0)
        return 0;
    return &pld[vpn0];
}

void handle_disk(uint64_t stval, PTE* pte_addr, int cpuid)
{
    PTE pte = (*pte_addr);
    uint64_t pa_kva = get_kva(pte);
    pcb_t* cur;
    if(cpuid==0)
        cur = current_running0;       
    else if(cpuid==1)
        cur = current_running1;
    allocpid = cur->pid;
    uint64_t newmem_addr = allocmem(1, stval);
    map(stval, kva2pa(newmem_addr), (pa2kva(cur->pgdir << 12)));
    getbackdisk(stval, newmem_addr);
    (*pte_addr) = (*pte_addr) | 1;
}

void handle_pf(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid)
{
    pcb_t* cur;
    if(cpuid==0)
        cur = current_running0;
    else if(cpuid==1)
        cur = current_running1;

    PTE* pte_addr = check_pf(stval, (pa2kva(cur->pgdir << 12)));
    if(pte_addr != 0)
    {
        PTE va_pte = *pte_addr;
        if(cause == EXCC_STORE_PAGE_FAULT)
        {
            //child process
            allocpid = cur->pid;
            if(cur->father_pid!=cur->pid)
            {
                fork_page_helper(stval, (pa2kva(cur->pgdir << 12)), (pa2kva(pcb[cur->father_pid].pgdir << 12)));
            }
            else
            {
                //father process
                for(int i=0; i<cur->child_num; i++)
                {
                    fork_page_helper(stval, (pa2kva(pcb[cur->child_pid[i]].pgdir << 12)), (pa2kva(cur->pgdir << 12)));
                }
            }
            *pte_addr = (*pte_addr) | (3<<6);
            *pte_addr = (*pte_addr) | (1<<2);
        }
        else if(cause == EXCC_INST_PAGE_FAULT)
            *pte_addr = (*pte_addr) | (3<<6);   
        else if((((va_pte>>6)&1)==0)&&(cause == EXCC_LOAD_PAGE_FAULT))
            *pte_addr = (*pte_addr) | (1<<6);
        else if((va_pte&1)==0) //physical frame was on disk
            handle_disk(stval, pte_addr, cpuid);
    }
    else
    {   //No virtual-physical map
        allocpid = cur->pid;
        alloc_page_helper(stval, (pa2kva(cur->pgdir << 12)));
        local_flush_tlb_all();
    }
}

void handle_irq(regs_context_t *regs, int irq)
{
    int flag = 0;
    if(flag==0)
        flag = check_send();
    if(flag==0)
        flag = check_recv();

    u32 RegISR = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                XEMACPS_ISR_OFFSET);
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, 
            XEMACPS_ISR_OFFSET,
            RegISR);
    plic_irq_eoi(irq);
    
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for(int i=0;i<IRQC_COUNT;i++)
    {
        irq_table[i] = handle_other;
    }
    irq_table[IRQC_S_TIMER] = handle_int;
    irq_table[9] = plic_handle_irq;
    for(int i=0;i<EXCC_COUNT;i++)
    {
        exc_table[i] = handle_other;
    }
    exc_table[EXCC_SYSCALL] = handle_syscall;
    exc_table[EXCC_LOAD_PAGE_FAULT] = handle_pf;
    exc_table[EXCC_STORE_PAGE_FAULT] = handle_pf;
    exc_table[EXCC_INST_PAGE_FAULT] = handle_pf;
    //setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause, uint64_t cpuid)
{
    prints("cpuid: %d\n", cpuid);
    
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            prints("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        prints("\n\r");
    }
    prints("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    prints("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    prints("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    prints("[Backtrace]\n\r");
    prints("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        prints("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
