/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <os/stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <user_programs.h>
#include <os/lock.h>
#include <pgtable.h>
#include <os/elf.h>
#include <csr.h>
#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#include <assert.h>

extern void ret_from_exception();
extern void __global_pointer$();

int freepidnum = 0;
int nowpid = 1;
int glmask = 3;
int glvalid = 0;
ptr_t address_base = 0xffffffc050504000lu;
pid_t freepid[NUM_MAX_TASK];
typedef void (*void_task)();
pid_t nextpid()
{
    if(freepidnum!=0)
        return freepid[freepidnum--];
    else return nowpid++;
}
ptr_t get_kernel_address(pid_t pid)
{
    return address_base + (pid + 1) * 0x2000;
}

ptr_t get_user_address(pid_t pid)
{
    return address_base + pid * 0x2000 + 0x1000;
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char** argv)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    uint64_t user_stack_kva = kernel_stack - 0x1000;

    for(int i=5;i<32;i++)
        pt_regs->regs[i] = 0;
    if(argv!=NULL)
    {
        char** u_argv = (char**)(user_stack_kva - 100);
        for(int i=0; i<argc; i++)
        {
            char* s = (char*)(user_stack_kva - 100 - 20 * (i+1));
            char* t = argv[i];
            int j = 0;
            for(j; t[j]!='\0'; j++)
                s[j] = t[j];
            s[j] = '\0';
            u_argv[i] = (char*)(user_stack - 100 - 20 * (i+1));
            //prints("%x\n",u_argv[i]);
        }
        pcb->user_sp = user_stack - 256;
        //prints("sp:%x\n", pcb->user_sp);
        pt_regs->regs[11] = user_stack - 100;
    }
    else 
        pcb->user_sp = user_stack;
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pt_regs->regs[0] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    if(pcb->type==USER_PROCESS)
        pt_regs->regs[3] = __global_pointer$;
    else if(pcb->type==USER_THREAD)
    {
        regs_context_t *cur_regs =
        (regs_context_t *)(current_running->kernel_sp + sizeof(switchto_context_t));
        pt_regs->regs[3] = cur_regs->regs[3];   
    }
    pt_regs->regs[4] = pcb; 
    pt_regs->regs[10] = argc;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0x40020;
    pt_regs->sbadaddr = 0;
    pt_regs->scause = 0;

    switchto_context_t *sw_regs = 
        (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[0] = ret_from_exception;
    //sw_regs->regs[1] = pcb->user_sp;
}

static void fork_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, pcb_t *pcb)
{
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    regs_context_t *cur_regs =
        (regs_context_t *)(current_running->kernel_sp + sizeof(switchto_context_t));
    ptr_t cur_address = get_user_address(current_running->pid);
    ptr_t off = 0x1000 - 1;
    char* p = (char*)(user_stack -1);
    char* q = (char*)(cur_address -1);
    //i is unsigned
    //printk("here1\n");
    for(ptr_t i=off+1;i>0;i--)
    {
        *p = *q; 
        p--;
        q--;
    }
    pcb->user_sp = current_running->user_sp;
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    for(int i=0;i<32;i++)
        pt_regs->regs[i] = cur_regs->regs[i];
    //tp
    pt_regs->regs[4] = pcb;
    pt_regs->regs[2] = pcb->user_sp;
    //a0
    pt_regs->regs[10] = 0;
    //pt_regs->regs[8] = user_stack;
    pt_regs->sepc = cur_regs->sepc;
    pt_regs->sstatus = cur_regs->sstatus;
    pt_regs->sbadaddr = cur_regs->sbadaddr;
    pt_regs->scause = cur_regs->scause;

    switchto_context_t *sw_regs = 
        (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));
    sw_regs->regs[0] = ret_from_exception;
    //sw_regs->regs[1] = pcb->user_sp;
}
void tostr(char* s, int num)
{
    int i=num;
    int len=0;
    char temp;
    do
    {
        s[len++]=(i%10) + '0';
        i/=10;
    }while(i!=0);
    for(i=0;i<len/2;i++)
    {
        temp = s[i];
        s[i] = s[len-1-i];
        s[len-1-i] = temp;
    }
    s[len]='\0';
}

int ps()
{
    int num=0;
    char numstr[5];
    char istr[5];
    for(int i=0;i<NUM_MAX_TASK;i++)
    {
        if(pcb[i].status!=TASK_EXITED)
        {
            if(num == 0)
                prints("[PROCESS TABLE]\n");
            prints("[%d] PID : %d, TID : %d", num, pcb[i].fpid, pcb[i].tid);// pcb[i].tid
            switch (pcb[i].status)
            {
            case TASK_RUNNING:
                screen_write(" STATUS : RUNNING");
                break;
            case TASK_BLOCKED:
                screen_write(" STATUS : BLOCKED");
                break;
            case TASK_ZOMBIE:
                screen_write(" STATUS : ZOMBIE");
                break;
            case TASK_READY:
                screen_write(" STATUS : READY");
                break;
            default:
                break;
            }
            prints(" MASK : 0x%d",pcb[i].cpuid);
            if(pcb[i].status==TASK_RUNNING)
            {
                screen_write(" on Core ");
                switch (pcb[i].cpuid)
                {
                    case 1:
                        screen_write("0\n");
                        break;
                    case 2:
                        screen_write("1\n");
                        break;
                    case 3:
                        if(&pcb[i]==current_running0)
                            screen_write("0\n");
                        else if(&pcb[i]==current_running1)
                            screen_write("1\n");
                        break;
                    default:
                        break;
                }
            }else
                screen_write("\n");
            num++;
        }
    }
    return num + 1;
}

int fs()
{
    prints("[EXECABLE LIST]\n");
    for(int i=1; i<ELF_FILE_NUM; i++)
    {
        prints("%d: %s", i, elf_files[i].file_name);
        screen_write("\n");
    }
    return ELF_FILE_NUM;
}

int do_kill(pid_t pid)
{
    if(pcb[pid].status==TASK_EXITED || pid == 0)
        return 0;
    if(pcb[pid].status==TASK_RUNNING||current_running0==&pcb[pid]||current_running1==&pcb[pid])
    {
        pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
        pcb[pid].killed = 1;
        return 1;
    }
    for(int i=0;i<pcb[pid].locksum;i++)
    {
        do_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for(int i=0; i<pcb[pid].threadsum; i++)
    {
        do_kill(pcb[pid].threadid[i]);
        //prints("kill %d\n", pcb[pid].threadid[i]);
    }
    pcb[pid].status = TASK_EXITED;
    //get back stack
    freepidnum++;
    freepid[freepidnum] = pid;
    if(pcb[pid].type!=USER_THREAD)
        getback_page(pid);
    while(pcb[pid].wait_list.next!=&pcb[pid].wait_list)
        do_unblock(&pcb[pid].wait_list,UNBLOCK_FROM_QUEUE);
    if(ready_queue==&pcb[pid].list)
        ready_queue=ready_queue->next;
    if(ready_queue0==&pcb[pid].list)
        ready_queue0=ready_queue0->next;
    if(ready_queue1==&pcb[pid].list)
        ready_queue1=ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    return 1;
}

void do_exit()
{
    int id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    pid_t pid=current_running->pid;
    //shell not allowed
    if(pid==0) return;
    pcb[pid].killed = 0;
    for(int i=0;i<pcb[pid].locksum;i++)
    {
        do_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for(int i=0; i<pcb[pid].threadsum; i++)
    {
        //prints("exit %d\n", pcb[pid].threadid[i]);
        do_kill(pcb[pid].threadid[i]);
        do_waitpid(pcb[pid].threadid[i]);
    }
    if(pcb[pid].mode==ENTER_ZOMBIE_ON_EXIT)
    {
        pcb[pid].status = TASK_ZOMBIE;
    }    
    else if(pcb[pid].mode==AUTO_CLEANUP_ON_EXIT)
    {
        pcb[pid].status = TASK_EXITED;
        freepidnum++;
        freepid[freepidnum] = pid;
        if(pcb[pid].type!=USER_THREAD)
            getback_page(pid);
    }
    while(pcb[pid].wait_list.next!=&pcb[pid].wait_list)
        do_unblock(&pcb[pid].wait_list,UNBLOCK_FROM_QUEUE);
    if(ready_queue==&pcb[pid].list)
        ready_queue=ready_queue->next;
    if(ready_queue0==&pcb[pid].list)
        ready_queue0=ready_queue0->next;
    if(ready_queue1==&pcb[pid].list)
        ready_queue1=ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    if(id==0)
        current_running0=&pid0_pcb;
    else if(id==1)
        current_running1=&pid0_pcb2;
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    do_scheduler();
}

int do_waitpid(pid_t pid)
{
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    while (pcb[pid].status!=TASK_EXITED && pcb[pid].status!=TASK_ZOMBIE)
    {
        do_block(&current_running->list, &pcb[pid].wait_list);
        do_scheduler();
    }
    if(pcb[pid].status==TASK_ZOMBIE)
    {
        freepidnum++;
        freepid[freepidnum] = pid;
        pcb[pid].status=TASK_EXITED;
    }
    return 1;
}

pid_t getpid()
{
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    return current_running->pid;
}

int do_taskset(int pid, int mask)
{
    int id = get_current_cpu_id();
    if(pid == 0)
    {
        if(mask != 3 && mask != 1+id)
        {
            glmask = mask;
            glvalid = 1;
            return 1;
        }
        else
            glvalid = 0;
    }
    if(pid>NUM_MAX_TASK||pcb[pid].status==TASK_EXITED||pcb[pid].status==TASK_ZOMBIE)
        return 0;
    pcb[pid].cpuid = mask;
    return 1;
}

//can shu you dian shao
pid_t spawn(task_info_t* info, void *arg, spawn_mode_t mode)
{
    current_running = get_current_cpu_id()==0? current_running0 : current_running1;
    int i=nextpid();
    list_node_t* p;
    pcb[i].type = info->type;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    pcb[i].mode = mode;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].pgdir = 0;
    if(glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    pcb[i].locksum = 0;
    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack = kernel_stack - 0x1000;
    init_pcb_stack(kernel_stack, 
        user_stack, (uintptr_t)(info->entry_point), &pcb[i], arg, NULL);
    p=&pcb[i].list;
    //add pcb to the ready 
    ready_queue->prev->next = p;
    p->next=ready_queue;
    p->prev=ready_queue->prev;
    ready_queue->prev = p;
    //ready_queue=p;
    return i;
}

int mthread_create(pid_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    current_running = get_current_cpu_id()==0? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i=nextpid();

    pcb[fpid].threadid[pcb[fpid].threadsum] = i;
    pcb[fpid].threadsum++;
    pcb[i].father_pid = i;
    pcb[i].tid = pcb[fpid].threadsum;
    pcb[i].fpid = fpid;
    list_node_t* p;
    pcb[i].type = USER_THREAD;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    //pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if(glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    uint64_t user_stack_uva = USER_STACK_ADDR + 0x1000 * pcb[fpid].threadsum + 0x1000;
    pcb[i].pgdir = pcb[fpid].pgdir;
    map(user_stack_uva - 0x1000, kva2pa(user_stack_kva), (pa2kva(pcb[i].pgdir << 12)));
    init_pcb_stack(kernel_stack_kva, 
        user_stack_uva, start_routine, &pcb[i], arg, NULL);
    p=&pcb[i].list;
    //add pcb to the ready 
    ready_queue->prev->next = p;
    p->next=ready_queue;
    p->prev=ready_queue->prev;
    ready_queue->prev = p;
    //ready_queue=p;
    *thread = i;
    return 1;
}

extern int try_get_from_file(const char *file_name, unsigned char **binary, int *length);
pid_t exec(char* name, int argc, char** argv)
{
    int len = 0;
    unsigned char *binary = NULL;
    if(get_elf_file(name, &binary, &len)==0)
    {
        if(try_get_from_file(name, &binary, &len)==0)
            return 0;
    }
    current_running = get_current_cpu_id()==0? current_running0 : current_running1;
    int i=nextpid();
    list_node_t* p;
    pcb[i].fpid = i;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].tid = 0;
    pcb[i].list.pid = i;
    pcb[i].threadsum = 0;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    pcb[i].father_pid = i;
    if(glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    allocpid = i;
    PTE* pgdir = allocPage(1);
    clear_pgdir(pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_task =
        (void_task)load_elf(binary, 
         len, pgdir, alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);
    init_pcb_stack(kernel_stack_kva, 
        USER_STACK_ADDR, test_task, &pcb[i], argc, argv);
    pcb[i].pgdir = kva2pa(pgdir) >> NORMAL_PAGE_SHIFT;
    p=&pcb[i].list;
    //add pcb to the ready 
    ready_queue->prev->next = p;
    p->next=ready_queue;
    p->prev=ready_queue->prev;
    ready_queue->prev = p;
    //ready_queue=p;
    return i;
}

void init_net()
{
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
    uintptr_t _plic_addr = 
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
    // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    // xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId        = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);
    
    long status = EmacPsInit(&EmacPsInstance);
    if (status != XST_SUCCESS) {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }
}

extern map_fs_space();
extern void init_fs();

static void init_pcb()
{
    allocpid = 0;
    init_fs();
    init_net();
    for(int i=0;i<NUM_MAX_TASK;i++)
    {
        pcb[i].status=TASK_EXITED;
        pcb[i].first = 1;
    }
    pcb[0].type = USER_PROCESS;
    pcb[0].pid = 0;
    pcb[0].fpid = 0;
    pcb[0].tid = 0;
    pcb[0].threadsum = 0;
    pcb[0].list.pid = 0;
    pcb[0].list.next = &pcb[0].list;
    pcb[0].list.prev = &pcb[0].list;
    pcb[0].status = TASK_READY;
    pcb[0].cursor_x = 1;
    pcb[0].cursor_y = 1;
    pcb[0].priority = 0;
    pcb[0].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[0].wait_list.next = &pcb[0].wait_list;
    pcb[0].wait_list.prev = &pcb[0].wait_list;
    pcb[0].cpuid = 3;
    pcb[0].locksum = 0;
    pcb[0].child_num = 0;
    pcb[0].killed = 0;
    pcb[0].father_pid = 0;
    ptr_t kernel_stack = get_kernel_address(0);
    ptr_t user_stack = kernel_stack - 0x2000;
    allocpid = 0;
    PTE* pgdir = allocPage(1);
    clear_pgdir(pgdir);
    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file("shell",&binary,&len);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_shell =
        (void_task)load_elf(binary, 
         len, pgdir, alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack), pgdir);
    init_pcb_stack(kernel_stack, 
        USER_STACK_ADDR, test_shell, &pcb[0], 0, NULL);
    pcb[0].pgdir = kva2pa(pgdir) >> NORMAL_PAGE_SHIFT;
    current_running0 = &pid0_pcb;
    ready_queue = &pcb[0].list;
    ready_queue0 = &pcb[0].list;
    ready_queue1 = &pcb[0].list;
    current_running1 = &pid0_pcb2;
    timers.next = &timers;
    timers.prev = &timers;
    send_queue.next = &send_queue;
    send_queue.prev = &send_queue;
    recv_queue.next = &recv_queue;
    recv_queue.prev = &recv_queue;
}

static int do_fork(int prior)
{
    current_running = get_current_cpu_id()==0? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i=nextpid();

    pcb[fpid].child_pid[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;
    pcb[i].father_pid = fpid;
    pcb[i].tid = 0;
    pcb[i].fpid = i;
    list_node_t* p;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    //pcb[i].priority = prior;
    pcb[i].priority = 0;
    pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    //pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if(glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    
    allocpid = i;
    PTE* pgdir = allocPage(1);
    clear_pgdir(pgdir);
    
    fork_pcb_stack(kernel_stack_kva, kernel_stack_kva - 0x1000, &pcb[i]);
    fork_pgtable(pgdir, (pa2kva(pcb[fpid].pgdir << 12)));
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);
    
    pcb[i].pgdir = kva2pa(pgdir) >> NORMAL_PAGE_SHIFT;
    
    p=&pcb[i].list;
    //add pcb to the ready 
    ready_queue->prev->next = p;
    p->next=ready_queue;
    p->prev=ready_queue->prev;
    ready_queue->prev = p;
    //do_scheduler();
    return i;
}

void do_net_port(int id)
{
    current_running = get_current_cpu_id()==0? current_running0 : current_running1;
    if(id==50001)
        current_running->port=1;
    else if(id==58688)
        current_running->port=2;
}

extern void do_mkfs(int func);
extern void do_statfs();
extern void do_ls();
extern int do_rmdir(char *name);
extern int do_mkdir(char *name);
extern int do_cd(char* name);
extern int do_touch(char* name);
extern int do_cat(char* name);
extern int do_fopen(char* name, int access);
extern int do_fread(int fid, char *buff, int size);
extern int do_fwrite(int fid, char *buff, int size);
extern void do_fclose(int fid);
extern int do_ln(char* name, char *path);
extern int do_rm(char *name);
extern int do_lseek(int fid, int offset, int whence);

static void init_syscall(void)
{
    syscall[SYSCALL_SLEEP] = do_sleep;
    syscall[SYSCALL_YIELD] = do_scheduler;
    syscall[SYSCALL_READ] = port_read;
    syscall[SYSCALL_WRITE] = screen_write;
    syscall[SYSCALL_FORK] = do_fork;
    syscall[SYSCALL_CURSOR] = screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = screen_reflush;
    syscall[SYSCALL_GET_TICK] = get_ticks;
    syscall[SYSCALL_GET_TIMEBASE] = get_time_base;
 
    net_poll_mode = 1;
    // xemacps_example_main();
    syscall[SYSCALL_LOCK_INIT] = do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQUIRE] = do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE] = do_mutex_lock_release;
    syscall[SYSCALL_GET_WALL_TIME] = get_wall_time;
    syscall[SYSCALL_CLEAR] = screen_clear;
    syscall[SYSCALL_PS] = ps;
    syscall[SYSCALL_SPAWN] = spawn;
    syscall[SYSCALL_KILL] = do_kill;
    syscall[SYSCALL_EXIT] = do_exit;
    syscall[SYSCALL_WAITPID] = do_waitpid;
    syscall[SYSCALL_GETPID] = getpid;
    syscall[SYSCALL_SEMAPHORE_INIT] = do_semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP] = do_semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN] = do_semaphore_down;
    syscall[SYSCALL_SEMAPHORE_DESTROY] = do_semaphore_destroy;
    syscall[SYSCALL_BARRIER_INIT] = do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = do_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY] = do_barrier_destroy;
    syscall[SYSCALL_MBOX_OPEN] = do_mbox_open;
    syscall[SYSCALL_MBOX_SEND] = do_mbox_send;
    syscall[SYSCALL_MBOX_CLOSE] = do_mbox_close;
    syscall[SYSCALL_MBOX_RECV] = do_mbox_recv;
    syscall[SYSCALL_TASKSET] = do_taskset;
    syscall[SYSCALL_TEST_RECV] = do_test_recv;
    syscall[SYSCALL_TEST_SEND] = do_test_send;
    syscall[SYSCALL_EXEC] = exec;
    syscall[SYSCALL_FS] = fs;
    syscall[SYSCALL_MTHREAD_CREATE] = mthread_create;
    syscall[SYSCALL_GETPA] = do_getpa;
    syscall[SYSCALL_SHMPAGE_GET] = shm_page_get;
    syscall[SYSCALL_SHMPAGE_DT] = shm_page_dt;
    syscall[SYSCALL_NET_RECV] = do_net_recv;
    syscall[SYSCALL_NET_SEND] = do_net_send;
    syscall[SYSCALL_NET_IRQ_MODE] = do_net_irq_mode;
    syscall[SYSCALL_NET_PORT] = do_net_port;
    syscall[SYSCALL_MKFS] = do_mkfs;
    syscall[SYSCALL_STATFS] = do_statfs;
    syscall[SYSCALL_CD] = do_cd;
    syscall[SYSCALL_MKDIR] = do_mkdir;
    syscall[SYSCALL_RMDIR] = do_rmdir;
    syscall[SYSCALL_LS] = do_ls;
    syscall[SYSCALL_TOUCH] = do_touch;
    syscall[SYSCALL_CAT] = do_cat;
    syscall[SYSCALL_FOPEN] = do_fopen;
    syscall[SYSCALL_FREAD] = do_fread;
    syscall[SYSCALL_FWRITE] = do_fwrite;
    syscall[SYSCALL_FCLOSE] = do_fclose;
    syscall[SYSCALL_LN] = do_ln;
    syscall[SYSCALL_RM] = do_rm;
    syscall[SYSCALL_LSEEK] = do_lseek;
    // initialize system call table.
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    int id = get_current_cpu_id();
    if(id == 1)
    {
        setup_exception();
        cancelpg(pa2kva(PGDIR_PA));
        sbi_set_timer(0);
    }
    // init Process Control Block (-_-!)
    init_pcb();
    //printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // init system call table (0_0)
    init_syscall();
    //printk("> [INIT] System call initialized successfully.\n\r");

    //fdt_print(riscv_dtb);
    init_exception();
    //printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init screen (QAQ)
    init_screen();
    //printk("> [INIT] SCREEN initialization succeeded.\n\r");
    // TODO:
    // Setup timer interrupt and enable all interrupt
    // init interrupt (^_^)
    setup_exception();
    sbi_set_timer(0);
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        // do_scheduler();
    };
    return 0;
}
