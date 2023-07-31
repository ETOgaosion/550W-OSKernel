# 进程管理

本节主要介绍进程数据结构管理与进程调度

## PCB数据结构

决赛中glibc所需要系统调用的需求已经使pcb结构体到达足够丰富的体量，需要记录和维护足够多的信息以提供可用的支持。

```C
typedef struct pcb {
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    /* previous, next pointer */
    list_node_t list;

    regs_context_t *save_context;
    switchto_context_t *switch_context;

    uint64_t core_id;

    bool in_use;
    elf_info_t elf;
    bool dynamic;

    /* process id */
    char name[NUM_MAX_PCB_NAME];
    char cmd[NUM_MAX_PCB_CMD];
    pid_t pid; // real offset of pcb[]
    pid_t tid;
    pid_t pgid;
    gids_t gid;
    uids_t uid;
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

    int sched_policy;
    prior_t priority;

    uint8_t core_mask[CPU_SET_SIZE];

    unsigned int personality;

    uint64_t pgdir;

    /* signal handler */
    bool handling_signal;
    unsigned long flags;

    sigaction_t *sigactions;
    uint64_t sig_recv;
    uint64_t sig_pend;
    uint64_t sig_mask;
    uint64_t prev_mask;

    int locksum;
    int lock_ids[NUM_MAX_LOCK];
    void *chan;

    pcb_mbox_t *mbox;

    list_head fd_head;
    /* time */
    kernel_timeval_t stime_last; // last time into kernel
    kernel_timeval_t utime_last; // last time out kernel
    kernel_timeval_t real_time_last;
    pcbtimer_t timer;
    kernel_clock_t dead_child_stime;
    kernel_clock_t dead_child_utime;
    kernel_itimerval_t real_itime;
    kernel_itimerval_t virt_itime;
    kernel_itimerval_t prof_itime;

    nanotime_val_t real_time_last_nano;
    kernel_full_clock_t cputime_id_clock;

    uint64_t start_tick;

    rusage_t resources;

    cap_user_data_t cap[2];

    uintptr_t arg_start;
    uintptr_t arg_end;
    uintptr_t env_start;
    uintptr_t env_end;
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