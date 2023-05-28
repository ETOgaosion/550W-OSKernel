#pragma once

#include <asm/context.h>
#include <common/elf.h>
#include <lib/list.h>
#include <os/cpu.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/sync.h>
#include <os/time.h>

#define NUM_MAX_PRCESS 16
#define NUM_MAX_CHILD 5
#define NUM_MAX_CHILD_THREADS 5
#define NUM_MAX_TASK (1 + NUM_MAX_CHILD + NUM_MAX_CHILD_THREADS) * NUM_MAX_PRCESS
#define NUM_MAX_LOCK 16
#define NUM_MAX_MBOX 16

#define WNOHANG 0x00000001
#define WUNTRACED 0x00000002
#define WSTOPPED WUNTRACED
#define WEXITED 0x00000004
#define WCONTINUED 0x00000008
#define WNOWAIT 0x01000000 /* Don't reap, just poll status.  */

#define __WNOTHREAD 0x20000000 /* Don't wait on children of other threads in this group */
#define __WALL 0x40000000      /* Wait on all children, regardless of type */
#define __WCLONE 0x80000000    /* Wait only on non-SIGCHLD children */

#define PRIO_MIN (-20)
#define PRIO_MAX 20

#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

/*
 * cloning flags:
 */
#define CSIGNAL 0x000000ff              /* signal mask to be sent at exit */
#define CLONE_VM 0x00000100             /* set if VM shared between processes */
#define CLONE_FS 0x00000200             /* set if fs info shared between processes */
#define CLONE_FILES 0x00000400          /* set if open files shared between processes */
#define CLONE_SIGHAND 0x00000800        /* set if signal handlers and blocked signals shared */
#define CLONE_PIDFD 0x00001000          /* set if a pidfd should be placed in parent */
#define CLONE_PTRACE 0x00002000         /* set if we want to let tracing continue on the child too */
#define CLONE_VFORK 0x00004000          /* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT 0x00008000         /* set if we want to have the same parent as the cloner */
#define CLONE_THREAD 0x00010000         /* Same thread group? */
#define CLONE_NEWNS 0x00020000          /* New mount namespace group */
#define CLONE_SYSVSEM 0x00040000        /* share system V SEM_UNDO semantics */
#define CLONE_SETTLS 0x00080000         /* create a new TLS for the child */
#define CLONE_PARENT_SETTID 0x00100000  /* set the TID in the parent */
#define CLONE_CHILD_CLEARTID 0x00200000 /* clear the TID in the child */
#define CLONE_DETACHED 0x00400000       /* Unused, ignored */
#define CLONE_UNTRACED 0x00800000       /* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID 0x01000000   /* set the TID in the child */
#define CLONE_NEWCGROUP 0x02000000      /* New cgroup namespace */
#define CLONE_NEWUTS 0x04000000         /* New utsname namespace */
#define CLONE_NEWIPC 0x08000000         /* New ipc namespace */
#define CLONE_NEWUSER 0x10000000        /* New user namespace */
#define CLONE_NEWPID 0x20000000         /* New pid namespace */
#define CLONE_NEWNET 0x40000000         /* New network namespace */
#define CLONE_IO 0x80000000             /* Clone io context */

typedef void (*void_task)();

typedef struct prior {
    long priority;
    uint64_t last_sched_time;
} prior_t;

typedef enum task_status {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
} task_status_t;

typedef enum task_type {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

typedef enum enqueue_way {
    ENQUEUE_LIST,
    ENQUEUE_TIMER_LIST,
} enqueue_way_t;

typedef enum dequeue_way {
    DEQUEUE_LIST,
    DEQUEUE_WAITLIST,
    DEQUEUE_WAITLIST_DESTROY,
    DEQUEUE_LIST_STRATEGY,
} dequeue_way_t;

typedef enum unblock_way {
    UNBLOCK_TO_LIST_FRONT,
    UNBLOCK_TO_LIST_BACK,
    UNBLOCK_ONLY,
    UNBLOCK_TO_LIST_STRATEGY,
} unblock_way_t;

typedef struct rusage {
    __kernel_timeval_t ru_utime; /* user time used */
    __kernel_timeval_t ru_stime; /* system time used */
    __kernel_long_t ru_maxrss;   /* maximum resident set size */
    __kernel_long_t ru_ixrss;    /* integral shared memory size */
    __kernel_long_t ru_idrss;    /* integral unshared data size */
    __kernel_long_t ru_isrss;    /* integral unshared stack size */
    __kernel_long_t ru_minflt;   /* page reclaims */
    __kernel_long_t ru_majflt;   /* page faults */
    __kernel_long_t ru_nswap;    /* swaps */
    __kernel_long_t ru_inblock;  /* block input operations */
    __kernel_long_t ru_oublock;  /* block output operations */
    __kernel_long_t ru_msgsnd;   /* messages sent */
    __kernel_long_t ru_msgrcv;   /* messages received */
    __kernel_long_t ru_nsignals; /* signals received */
    __kernel_long_t ru_nvcsw;    /* voluntary context switches */
    __kernel_long_t ru_nivcsw;   /* involuntary " */
} rusage_t;

/* Process Control Block */
typedef struct pcb {
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;

    regs_context_t *save_context;
    switchto_context_t *switch_context;

    bool in_use;
    ELF_info_t elf;

    /* process id */
    pid_t pid;  // real offset of pcb[]
    pid_t fpid; // threads' fpid is process pid
    pid_t tid;
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

    uint8_t core_mask[CPU_SET_SIZE];

    uint64_t pgdir;

    int locksum;
    int lock_ids[NUM_MAX_LOCK];
    void *chan;

    pcb_mbox_t mbox;

    /* time */
    __kernel_timeval_t stime_last; // last time into kernel
    __kernel_timeval_t utime_last; // last time out kernel
    pcbtimer_t timer;
    __kernel_clock_t dead_child_stime;
    __kernel_clock_t dead_child_utime;

    rusage_t resources;
} pcb_t;

/* current running task PCB */
extern pcb_t *volatile current_running0;
extern pcb_t *volatile current_running1;
extern pcb_t *volatile *volatile current_running;
/* ready queue to run */
extern list_head ready_queue;
extern list_head block_queue;

extern pcb_t pcb[NUM_MAX_TASK];
// pcb_t kernel_pcb[NR_CPUS];
extern const ptr_t pid0_stack;
extern const ptr_t pid0_stack2;
extern pcb_t pid0_pcb;
extern pcb_t pid0_pcb2;

extern pid_t freepid[NUM_MAX_TASK];

void k_init_pcb();
extern void switch_to(pcb_t *prev, pcb_t *next);
long k_scheduler(void);
void k_block(list_head *, list_head *queue, enqueue_way_t way);
void k_unblock(list_head *, list_head *, unblock_way_t way);
long k_getpid(void);
void k_sleep(void *chan, spin_lock_t *lk);
void k_wakeup(void *chan);

long sys_sched_yield(void);
long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp);
long sys_spawn(const char *file_name);
long sys_fork(void);
long sys_exec(const char *file_name, const char *argv[], const char *envp[]);
long sys_execve(const char *file_name, const char *argv[], const char *envp[]);
long sys_clone(unsigned long flags, void *stack, void *arg, pid_t *parent_tid, void *tls, pid_t *child_tid);
long sys_kill(pid_t pid);
long sys_exit(int error_code);
long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru);
long sys_process_show();
long sys_setpriority(int which, int who, int niceval);
long sys_getpriority(int which, int who);
long sys_getpid(void);
long sys_getppid(void);
long sys_sched_setaffinity(pid_t pid, unsigned int len, const uint8_t *user_mask_ptr);
long sys_sched_getaffinity(pid_t pid, unsigned int len, uint8_t *user_mask_ptr);