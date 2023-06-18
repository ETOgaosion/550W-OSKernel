# 进程管理

本节主要介绍进程数据结构管理与进程调度

## PCB数据结构

```C
typedef struct pcb {
    /* kernel stack, used to save context */
    reg_t kernel_sp;    

    /* user stack */
    reg_t user_sp;     

    /* list used to form ready queue or block queue */
    list_node_t list;   

    /* saved context */
    regs_context_t *save_context;  
    switchto_context_t *switch_context; 

    /* if this PCB is in use */
    bool in_use;

    /* info of elf */
    ELF_info_t elf;

    /* informatino about process*/
    char name[NUM_MAX_PCB_NAME]; // name of process
    pid_t pid;  // real offset of pcb[]
    pid_t fpid; // process id
    pid_t tid; // thread id
    uint32_t *clear_ctid;
    pid_t father_pid;
    pid_t child_pids[NUM_MAX_CHILD];
    int child_num;
    int *child_stat_addrs[NUM_MAX_CHILD];
    int threadsum;
    int thread_ids[NUM_MAX_CHILD_THREADS];

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    int exit_status;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    prior_t priority;

    /* if allowed running on certain core */
    uint8_t core_mask[CPU_SET_SIZE];

    /* page table directory address */
    uint64_t pgdir;

    /* info about lock */
    int locksum;
    int lock_ids[NUM_MAX_LOCK];
    void *chan;

    /* mailbox */
    pcb_mbox_t *mbox;

    /* time */
    __kernel_timeval_t stime_last; // last time into kernel
    __kernel_timeval_t utime_last; // last time out kernel
    pcbtimer_t timer;
    __kernel_clock_t dead_child_stime;
    __kernel_clock_t dead_child_utime;

    rusage_t resources;
} pcb_t;
```

## 进程调度

进程调度通过k_scheduler函数进行

``` C
 if (!list_is_empty(&timers)) {
        check_sleeping(); //检查是否有睡眠的进程
    }
    pcb_t *curr = (*current_running);
    int cpuid = k_get_current_cpu_id();

    /* 维护ready queue队列，里面都是可以执行的进程 */
    if (curr->status == TASK_RUNNING && curr->pid >= 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_LIST);
    }

    /* 取出下一个可执行的进程 */
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_STRATEGY);
    
    /* 更新PCB和current_running的信息 */
    next_pcb->status = TASK_RUNNING;
    if (cpuid == 0) {
        current_running0 = next_pcb;
        current_running = &current_running0;
    } else {
        current_running1 = next_pcb;
        current_running = &current_running1;
    }

    /* 更新虚地址映射 */
    set_satp(SATP_MODE_SV39, (*current_running)->pid + 1, (*current_running)->pgdir);
    local_flush_tlb_all();

    /* 切换进程 */
    switch_to(curr, next_pcb);
    return 0;
```

为了便于实现，我们规划了两个内核进程来保证两个核都有可以调度的进程，避免陷入没有进程可以调度的局面，也避免更加复杂的判断。

而关于switch_to函数，主要作用是将函数的上下文进行保存和恢复：
在这个过程中将上一个进程的函数上下文保存起来，恢复下一个进程的函数上下文，从而完成“从进程A进入该函数，函数返回后执行进程B”的操作。