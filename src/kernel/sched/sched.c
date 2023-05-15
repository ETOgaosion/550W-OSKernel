#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/smp.h>

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

/* current running task PCB */
unsigned x = 0;

extern int check_wait_recv(int num_packet);

void mysrand(unsigned seed) { x = seed; }

int myrand() {
    long long tmp = 0x5deece66dll * x + 0xbll;
    x = tmp & 0x7fffffff;
    return x;
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
            do_unblock(p, UNBLOCK_THIS_PCB);
            p = q;
        } else
            p = p->next;
    }
}

int check_pcb(int cpuid) {
    int flag = 0;
    list_node_t *p;
    list_node_t *q;
    int id = 0;
    mysrand(get_ticks());
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

void do_scheduler(void) {
    int id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    if (current_running->killed)
        do_exit();
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

void do_sleep(uint32_t sleep_time) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    current_running->list.time = sleep_time;
    current_running->list.start_time = get_timer();
    do_block(&(current_running->list), &timers);
    do_scheduler();
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
}

void do_block(list_node_t *pcb_node, list_head *queue) {
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

void do_unblock(list_node_t *pcb_node, int type) {
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
