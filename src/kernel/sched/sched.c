#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/stack.h>
#include <common/elf.h>
#include <drivers/screen.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/smp.h>
#include <user/user_programs.h>

pid_t process_id = 1;
int cpu_lock = 0;
int allocpid = 0;

list_node_t *avalable0[20];
list_node_t *avalable1[20];

pcb_t *volatile current_running0;
pcb_t *volatile current_running1;
pcb_t *volatile current_running;

list_head *ready_queue;
list_head *ready_queue0;
list_head *ready_queue1;

pcb_t pcb[NUM_MAX_TASK];

const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE * 3 - 112 - 288;
const ptr_t pid0_stack2 = INIT_KERNEL_STACK + PAGE_SIZE * 4 - 112 - 288;
pcb_t pid0_pcb = {.pid = 0, .kernel_sp = (ptr_t)pid0_stack, .user_sp = (ptr_t)pid0_stack, .preempt_count = 0};
pcb_t pid0_pcb2 = {.pid = 0, .kernel_sp = (ptr_t)pid0_stack2, .user_sp = (ptr_t)pid0_stack2, .preempt_count = 0};

int freepidnum = 0;
int nowpid = 1;
int glmask = 3;
int glvalid = 0;

pid_t freepid[NUM_MAX_TASK];

pid_t nextpid() {
    if (freepidnum != 0)
        return freepid[freepidnum--];
    else
        return nowpid++;
}

unsigned x = 0;

extern int check_wait_recv(int num_packet);

void mysrand(unsigned seed) { x = seed; }

int myrand() {
    long long tmp = 0x5deece66dll * x + 0xbll;
    x = tmp & 0x7fffffff;
    return x;
}

void init_net() { printk("[net] Init Net is not neccessary\n\r"); }

extern void init_fs();

list_node_t send_queue;
list_node_t recv_queue;

void init_pcb() {
    allocpid = 0;
    init_fs();
    init_net();
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].status = TASK_EXITED;
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
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((uintptr_t)pgdir);
    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file("shell", &binary, &len);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_shell = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack), pgdir);
    init_pcb_stack(kernel_stack, USER_STACK_ADDR, (ptr_t)test_shell, &pcb[0], 0, NULL);
    pcb[0].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
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

int check_pcb(int cpuid) {
    int flag = 0;
    list_node_t *p;
    list_node_t *q;
    int id = 0;
    mysrand(sys_get_ticks());
    if (cpuid == 0) {
        // if(!(&pcb[ready_queue0->pid]==current_running1||pcb[ready_queue0->pid].cpuid==2))
        //    return 1;
        p = ready_queue;
        if (!(&pcb[p->pid] == current_running1 || pcb[p->pid].cpuid == 2)) {
            avalable0[flag] = p;
            flag++;
        }
        q = ready_queue;
        p = ready_queue->next;
        while (p != q) {
            if (!(&pcb[p->pid] == current_running1 || pcb[p->pid].cpuid == 2)) {
                // ready_queue0=p;
                avalable0[flag] = p;
                flag++;
            }
            p = p->next;
        }
        if (flag != 0) {
            id = myrand() % flag;
            ready_queue0 = avalable0[id];
        }
    } else /*if (cpuid == 1) */ {
        // if(!(&pcb[ready_queue1->pid]==current_running0||pcb[ready_queue1->pid].cpuid==1))
        //    return 1;
        p = ready_queue;
        if (!(&pcb[p->pid] == current_running0 || pcb[p->pid].cpuid == 1)) {
            avalable1[flag] = p;
            flag++;
        }
        q = ready_queue;
        p = ready_queue->next;
        while (p != q) {
            if (!(&pcb[p->pid] == current_running0 || pcb[p->pid].cpuid == 1)) {
                // ready_queue1=p;
                avalable1[flag] = p;
                flag++;
            }
            p = p->next;
        }
        if (flag != 0) {
            id = myrand() % flag;
            ready_queue1 = avalable1[id];
        }
    }
    return flag;
}

void sys_scheduler(void) {
    int id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    if (current_running->killed)
        sys_exit();
    while (check_pcb(id) == 0) {
        atomic_swap(0, (ptr_t)&cpu_lock);
        while (atomic_swap(1, (ptr_t)&cpu_lock) != 0)
            ;
        id = get_current_cpu_id();
    }
    id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    pcb_t *prev = current_running;
    if (current_running->status == TASK_RUNNING)
        prev->status = TASK_READY;
    // Modify the current_running pointer.
    if (id == 0) {
        current_running0 = &pcb[ready_queue0->pid];
        // ready_queue0 = ready_queue0->next;
    } else if (id == 1) {
        current_running1 = &pcb[ready_queue1->pid];
        // ready_queue1 = ready_queue1->next;
    }
    current_running = id == 0 ? current_running0 : current_running1;
    current_running->status = TASK_RUNNING;
    // ready_queue=ready_queue->next;
    if (current_running->first) {
        current_running->first = 0;
        set_satp(SATP_MODE_SV39, current_running->pid + 1, current_running->pgdir);
        local_flush_tlb_all();
        reset_irq_timer();
        atomic_swap(0, (ptr_t)&cpu_lock);
    } else {
        set_satp(SATP_MODE_SV39, current_running->pid + 1, current_running->pgdir);
        local_flush_tlb_all();
    }
    // prints("cur_do: %x\n", current_running);
    if (id == 0)
        switch_to(prev, current_running0);
    else
        switch_to(prev, current_running1);
}

void sys_sleep(uint32_t sleep_time) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    current_running->list.time = sleep_time;
    current_running->list.start_time = get_timer();
    k_block(&(current_running->list), &timers);
    sys_scheduler();
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
}

void check_sleeping() {
    list_node_t *p;
    list_node_t *q;
    uint64_t now_time;
    p = timers.next;
    while (p != &timers) {
        now_time = get_timer();
        if (now_time - p->start_time >= p->time) {
            q = p->next;
            k_unblock(p, UNBLOCK_THIS_PCB);
            p = q;
        } else
            p = p->next;
    }
}

void k_block(list_node_t *pcb_node, list_head *queue) {
    // block the pcb task into the block queue
    // delte pcb_node from original list
    if (ready_queue == pcb_node)
        ready_queue = ready_queue->next;
    if (ready_queue1 == pcb_node)
        ready_queue1 = ready_queue1->next;
    if (ready_queue0 == pcb_node)
        ready_queue0 = ready_queue0->next;
    pcb_node->prev->next = pcb_node->next;
    pcb_node->next->prev = pcb_node->prev;
    // list_node_t *p = queue;
    pcb_node->next = queue->next;
    queue->next->prev = pcb_node;
    queue->next = pcb_node;
    pcb_node->prev = queue;
    /*while(p->next!=NULL)
        p=p->next;
    p->next = pcb_node;
    pcb_node->prev = p;
    pcb_node->next = NULL;
    */
    pcb[pcb_node->pid].status = TASK_BLOCKED;
}

void k_unblock(list_node_t *pcb_node, int type) {
    // unblock the `pcb` from the block queue
    // empty list error
    list_node_t *p;
    if (type == UNBLOCK_FROM_QUEUE) {
        p = pcb_node->next;
        if (p == pcb_node)
            return;
    } else if (type == UNBLOCK_THIS_PCB)
        p = pcb_node;
    else
        return;
    pcb[p->pid].status = TASK_READY;
    // delte pcb from blocked queue
    p->prev->next = p->next;
    // if(p->next!=NULL)
    p->next->prev = p->prev;

    // add pcb to the ready queue
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
}

int sys_taskset(int pid, int mask) {
    int id = get_current_cpu_id();
    if (pid == 0) {
        if (mask != 3 && mask != 1 + id) {
            glmask = mask;
            glvalid = 1;
            return 1;
        } else
            glvalid = 0;
    }
    if (pid > NUM_MAX_TASK || pcb[pid].status == TASK_EXITED || pcb[pid].status == TASK_ZOMBIE)
        return 0;
    pcb[pid].cpuid = mask;
    return 1;
}

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int i = nextpid();
    list_node_t *p;
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
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    pcb[i].locksum = 0;
    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack = kernel_stack - 0x1000;
    /* TODO: use glibc argument passing standards to restructure */
    /* 1, arg is wrong, only for passing compilation */
    init_pcb_stack(kernel_stack, user_stack, (uintptr_t)(info->entry_point), &pcb[i], (uint64_t)arg, NULL);
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    return i;
}

int sys_kill(pid_t pid) {
    if (pcb[pid].status == TASK_EXITED || pid == 0)
        return 0;
    if (pcb[pid].status == TASK_RUNNING || current_running0 == &pcb[pid] || current_running1 == &pcb[pid]) {
        pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
        pcb[pid].killed = 1;
        return 1;
    }
    for (int i = 0; i < pcb[pid].locksum; i++) {
        sys_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for (int i = 0; i < pcb[pid].threadsum; i++) {
        sys_kill(pcb[pid].threadid[i]);
        // prints("kill %d\n", pcb[pid].threadid[i]);
    }
    pcb[pid].status = TASK_EXITED;
    // get back stack
    freepidnum++;
    freepid[freepidnum] = pid;
    if (pcb[pid].type != USER_THREAD)
        getback_page(pid);
    while (pcb[pid].wait_list.next != &pcb[pid].wait_list)
        k_unblock(&pcb[pid].wait_list, UNBLOCK_FROM_QUEUE);
    if (ready_queue == &pcb[pid].list)
        ready_queue = ready_queue->next;
    if (ready_queue0 == &pcb[pid].list)
        ready_queue0 = ready_queue0->next;
    if (ready_queue1 == &pcb[pid].list)
        ready_queue1 = ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    return 1;
}

void sys_exit() {
    int id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    pid_t pid = current_running->pid;
    // shell not allowed
    if (pid == 0)
        return;
    pcb[pid].killed = 0;
    for (int i = 0; i < pcb[pid].locksum; i++) {
        sys_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for (int i = 0; i < pcb[pid].threadsum; i++) {
        // prints("exit %d\n", pcb[pid].threadid[i]);
        sys_kill(pcb[pid].threadid[i]);
        sys_waitpid(pcb[pid].threadid[i]);
    }
    if (pcb[pid].mode == ENTER_ZOMBIE_ON_EXIT) {
        pcb[pid].status = TASK_ZOMBIE;
    } else if (pcb[pid].mode == AUTO_CLEANUP_ON_EXIT) {
        pcb[pid].status = TASK_EXITED;
        freepidnum++;
        freepid[freepidnum] = pid;
        if (pcb[pid].type != USER_THREAD)
            getback_page(pid);
    }
    while (pcb[pid].wait_list.next != &pcb[pid].wait_list)
        k_unblock(&pcb[pid].wait_list, UNBLOCK_FROM_QUEUE);
    if (ready_queue == &pcb[pid].list)
        ready_queue = ready_queue->next;
    if (ready_queue0 == &pcb[pid].list)
        ready_queue0 = ready_queue0->next;
    if (ready_queue1 == &pcb[pid].list)
        ready_queue1 = ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    if (id == 0)
        current_running0 = &pid0_pcb;
    else if (id == 1)
        current_running1 = &pid0_pcb2;
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    sys_scheduler();
}

int sys_waitpid(pid_t pid) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    while (pcb[pid].status != TASK_EXITED && pcb[pid].status != TASK_ZOMBIE) {
        k_block(&current_running->list, &pcb[pid].wait_list);
        sys_scheduler();
    }
    if (pcb[pid].status == TASK_ZOMBIE) {
        freepidnum++;
        freepid[freepidnum] = pid;
        pcb[pid].status = TASK_EXITED;
    }
    return 1;
}

int sys_process_show() {
    int num = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].status != TASK_EXITED) {
            if (num == 0)
                prints("[PROCESS TABLE]\n");
            prints("[%d] PID : %d, TID : %d", num, pcb[i].fpid,
                   pcb[i].tid); // pcb[i].tid
            switch (pcb[i].status) {
            case TASK_RUNNING:
                sys_screen_write(" STATUS : RUNNING");
                break;
            case TASK_BLOCKED:
                sys_screen_write(" STATUS : BLOCKED");
                break;
            case TASK_ZOMBIE:
                sys_screen_write(" STATUS : ZOMBIE");
                break;
            case TASK_READY:
                sys_screen_write(" STATUS : READY");
                break;
            default:
                break;
            }
            prints(" MASK : 0x%d", pcb[i].cpuid);
            if (pcb[i].status == TASK_RUNNING) {
                sys_screen_write(" on Core ");
                switch (pcb[i].cpuid) {
                case 1:
                    sys_screen_write("0\n");
                    break;
                case 2:
                    sys_screen_write("1\n");
                    break;
                case 3:
                    if (&pcb[i] == current_running0)
                        sys_screen_write("0\n");
                    else if (&pcb[i] == current_running1)
                        sys_screen_write("1\n");
                    break;
                default:
                    break;
                }
            } else
                sys_screen_write("\n");
            num++;
        }
    }
    return num + 1;
}

extern int try_get_from_file(const char *file_name, unsigned char **binary, int *length);
pid_t sys_exec(const char *name, int argc, char **argv) {
    int len = 0;
    unsigned char *binary = NULL;
    /* TODO: use FAT32 disk to read program */
    if (get_elf_file(name, &binary, &len) == 0) {
        if (try_get_from_file(name, &binary, &len) == 0)
            return 0;
    }
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int i = nextpid();
    list_node_t *p;
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
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    allocpid = i;
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((uintptr_t)pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_task = (void_task)load_elf(binary, len, (uintptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);
    init_pcb_stack(kernel_stack_kva, USER_STACK_ADDR, (ptr_t)test_task, &pcb[i], argc, argv);
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    return i;
}

int sys_mthread_create(pid_t *thread, void (*start_routine)(void *), void *arg) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i = nextpid();

    pcb[fpid].threadid[pcb[fpid].threadsum] = i;
    pcb[fpid].threadsum++;
    pcb[i].father_pid = i;
    pcb[i].tid = pcb[fpid].threadsum;
    pcb[i].fpid = fpid;
    list_node_t *p;
    pcb[i].type = USER_THREAD;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    // pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    uint64_t user_stack_uva = USER_STACK_ADDR + 0x1000 * pcb[fpid].threadsum + 0x1000;
    pcb[i].pgdir = pcb[fpid].pgdir;
    map(user_stack_uva - 0x1000, kva2pa(user_stack_kva), (pa2kva(pcb[i].pgdir << 12)));
    init_pcb_stack(kernel_stack_kva, (ptr_t)user_stack_uva, (ptr_t)start_routine, &pcb[i], (uint64_t)arg, NULL);
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    *thread = i;
    return 1;
}

int sys_fork(int prior) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i = nextpid();

    pcb[fpid].child_pid[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;
    pcb[i].father_pid = fpid;
    pcb[i].tid = 0;
    pcb[i].fpid = i;
    list_node_t *p;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    // pcb[i].priority = prior;
    pcb[i].priority = 0;
    pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    // pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;

    allocpid = i;
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((ptr_t)pgdir);

    fork_pcb_stack(kernel_stack_kva, kernel_stack_kva - 0x1000, &pcb[i]);
    fork_pgtable(pgdir, (pa2kva(pcb[fpid].pgdir << 12)));
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);

    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // sys_scheduler();
    return i;
}