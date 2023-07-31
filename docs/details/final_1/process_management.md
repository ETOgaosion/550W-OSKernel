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

进程调度通过k_scheduler函数进行，首先检查按时睡眠进程，而后需要先处理当前进/线程信号，为性能考虑，对于实时任务可直接返回继续执行，否则就要依赖调度策略选择适合执行的下一个任务，有部分情况需要跳过前一个进程的上下文存储环节：当前一个进程已经失效或者被execve操作替换时，上下文存储地址已经失效，可跳过存储环节。

``` C
long k_pcb_scheduler(bool voluntary, bool skip_first) {
    if (!list_is_empty(&timers)) {
        check_sleeping();
    }
    check_itimers();
    k_signal_handler();
    pcb_t *curr = (*current_running);
    if (curr->sched_policy == SCHED_BATCH && curr->status == TASK_RUNNING) {
        return 0;
    }
    int cpuid = k_smp_get_current_cpu_id();
    if (curr->status == TASK_RUNNING && curr->pid >= 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_LIST_PRIORITY);
    }
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_FIFO, curr->sched_policy);
    if (!next_pcb) {
        return 0;
    }
    if (!voluntary) {
        (*current_running)->resources.ru_nivcsw++;
    }
    // d_screen_pcb_move_cursor(screen_cursor_x, screen_cursor_y);
    // d_screen_load_curpcb_cursor();
    next_pcb->status = TASK_RUNNING;
    next_pcb->core_id = curr->core_id;
    if (cpuid == 0) {
        current_running0 = next_pcb;
        current_running = &current_running0;
    } else {
        current_running1 = next_pcb;
        current_running = &current_running1;
    }
    set_satp(SATP_MODE_SV39, (*current_running)->pid + 1, (*current_running)->pgdir);
    local_flush_tlb_all();
    bool skip_first_cal = (curr == next_pcb) || curr->status == TASK_EXITED;
    switch_to(curr, next_pcb, skip_first_cal | skip_first);
    return 0;
}
```

为了便于实现，我们规划了两个内核进程来保证两个核都有可以调度的进程，避免陷入没有进程可以调度的局面，也避免更加复杂的判断。

而关于switch_to函数，主要作用是将函数的上下文进行保存和恢复：
在这个过程中将上一个进程的函数上下文保存起来，恢复下一个进程的函数上下文，从而完成“从进程A进入该函数，函数返回后执行进程B”的操作。

glibc库所依赖的系统调用有很多，决赛需求的系统调用也较为丰富，系统调用按照功能分类如下：

1. 进程线程生命周期管理函数，这些系统调用必须实现，且需要较为严格按照Linux-glibc规范完成功能实现，如`clone`系统调用需要对flags做尽可能多的支持才能使线程行为正确。生命周期包括启动、复制、结束、等待、睡眠等。

```C
long sys_spawn(const char *file_name);
long sys_fork(void);
long sys_exec(const char *file_name, const char *argv[], const char *envp[]);
long sys_execve(const char *file_name, const char *argv[], const char *envp[]);
long sys_clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid);
long sys_kill(pid_t pid);
long sys_exit(int error_code);
long sys_exit_group(int error_code);
long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru);
long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp);
```

2. 按照UNIX万物皆文件的哲学理念，`ps`并不是一个系统调用，这个需求需要读取`/proc/[pid]/`目录下的关键文件从而获取进程详细信息，因此操作系统需要维护这个虚拟文件系统。虽然万物皆文件的思想也许对于统一接口有着出色设计，但未必是高效的实现方式，上述过程需要依赖过多的系统调用以获取所有进程信息，包括但不限于`getdents64, openat, readlinkat, read, close`，相当低效。本项目为用户态debug方便支持了一次系统调用可获取的进程线程信息。

```C
long sys_process_show();
```

3. 调度相关系统调用，分为优先级与调度策略设置。550W现阶段采用混合调度方式，支持round-robin, FIFO, batch等调度，调度策略以任务的调度方式为准，即高优先级的FIFO调度和batch任务会饿死低优先级的round-robin任务，本操作系统以支持高优先级和实时任务为先。

```C
long sys_setpriority(int which, int who, int niceval);
long sys_getpriority(int which, int who);
```

```C
long sys_sched_setparam(pid_t pid, sched_param_t *param);
long sys_sched_setscheduler(pid_t pid, int policy, sched_param_t *param);
long sys_sched_getscheduler(pid_t pid);
long sys_sched_getparam(pid_t pid, sched_param_t *param);
long sys_sched_setaffinity(pid_t pid, unsigned int len, const uint8_t *user_mask_ptr);
long sys_sched_getaffinity(pid_t pid, unsigned int len, uint8_t *user_mask_ptr);
long sys_sched_yield(void);
long sys_sched_get_priority_max(int policy);
long sys_sched_get_priority_min(int policy);
```

4. 进程线程标识id管理，本操作系统目前并没有支持用户管理、session等，大部分的id实现都是从简，希望pcb结构体大小得到一定限制，

```C
long sys_set_tid_address(int *tidptr);
long sys_setgid(gid_t gid);
long sys_setuid(uid_t uid);
long sys_setresuid(uid_t ruid, uid_t euid, uid_t suid);
long sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
long sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid);
long sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
long sys_setpgid(pid_t pid, pid_t pgid);
long sys_getpgid(pid_t pid);
long sys_getsid(pid_t pid);
long sys_setsid(void);
long sys_getpid(void);
long sys_getppid(void);
long sys_getuid(void);
long sys_geteuid(void);
long sys_getgid(void);
long sys_getegid(void);
long sys_gettid(void);
```

如上述pcb结构体所示，与id相关的域有：

```C
pid_t pid; // real offset of pcb[]
pid_t tid;
pid_t pgid;
gids_t gid;
uids_t uid;
uint32_t *clear_ctid;
```

`pid`为pcb数组中偏移量，`tid`为线程专有，线程`tid`与`pid`一致，若pcb存在于进/线程组，若为首位主进/线程，`pgid`, `gid`与`pid`一致，由于目前只有单一root用户，`uid`始终为0

5. 进程线程资源管理，包含设置与获取

```C
long sys_getrusage(int who, rusage_t *ru);
long sys_prlimit64(pid_t pid, unsigned int resource, const rlimit64_t *new_rlim, rlimit64_t *old_rlim);
```

6. 其余杂项系统调用

```C
long sys_personality(unsigned int personality);
long sys_unshare(unsigned long unshare_flags);
long sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
```

```C
long sys_capget(cap_user_header_t *header, cap_user_data_t *dataptr);
long sys_capset(cap_user_header_t *header, const cap_user_data_t *data);
```