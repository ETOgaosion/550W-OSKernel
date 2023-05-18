#pragma once

#include <asm/context.h>
#include <common/types.h>
#include <lib/list.h>
#include <os/mm.h>
#include <os/time.h>

#define NUM_MAX_PRCESS 16
#define NUM_MAX_CHILD 5
#define NUM_MAX_CHILD_THREADS 5
#define NUM_MAX_TASK (1 + NUM_MAX_CHILD + NUM_MAX_CHILD_THREADS) * NUM_MAX_PRCESS
#define NUM_MAX_LOCK 16
#define NUM_MAX_MBOX 16

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002
#define WSTOPPED	WUNTRACED
#define WEXITED		0x00000004
#define WCONTINUED	0x00000008
#define WNOWAIT		0x01000000	/* Don't reap, just poll status.  */

#define __WNOTHREAD	0x20000000	/* Don't wait on children of other threads in this group */
#define __WALL		0x40000000	/* Wait on all children, regardless of type */
#define __WCLONE	0x80000000	/* Wait only on non-SIGCHLD children */

#define	PRIO_MIN	(-20)
#define	PRIO_MAX	20

#define	PRIO_PROCESS	0
#define	PRIO_PGRP	1
#define	PRIO_USER	2

#define ENQUEUE_LIST                0
#define ENQUEUE_PCB                 1
#define DEQUEUE_LIST                0
#define DEQUEUE_WAITLIST            1
#define DEQUEUE_WAITLIST_DESTROY    2
#define DEQUEUE_LIST_STRATEGY       3

#define UNBLOCK_TO_LIST_FRONT       0
#define UNBLOCK_TO_LIST_BACK        1
#define UNBLOCK_ONLY                2
#define UNBLOCK_TO_LIST_STRATEGY    3

typedef void (*void_task)();

typedef struct prior
{
    long priority;
    uint64_t last_sched_time;
} prior_t;

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb {
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;
    /* process id */
    pid_t pid;          // real offset of pcb[]
    pid_t fpid;         // threads' fpid is process pid
    pid_t tid;
    pid_t father_pid;
    pid_t child_pid[NUM_MAX_CHILD];
    int child_num;
    int *child_stat_addrs[NUM_MAX_CHILD];
    int threadsum;
    int threadid[NUM_MAX_CHILD_THREADS];

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    prior_t priority;

    int locksum;
    int lockid[NUM_MAX_LOCK];

    int mboxsum;
    int mboxid[NUM_MAX_MBOX];

    int core_mask;
    uint64_t pgdir;
    int port;

    /* time */
    __kernel_time_t stime;
    __kernel_time_t stime_last;     // last time into kernel
    __kernel_time_t utime;
    __kernel_time_t utime_last;     // last time out kernel
    pcbtimer_t timer;
} pcb_t;

/* task information, used to init PCB */
typedef struct task_info {
    ptr_t entry_point;
    task_type_t type;
} task_info_t;

typedef struct rusage {
	__kernel_timeval_t ru_utime;	/* user time used */
	__kernel_timeval_t ru_stime;	/* system time used */
	__kernel_long_t	ru_maxrss;	/* maximum resident set size */
	__kernel_long_t	ru_ixrss;	/* integral shared memory size */
	__kernel_long_t	ru_idrss;	/* integral unshared data size */
	__kernel_long_t	ru_isrss;	/* integral unshared stack size */
	__kernel_long_t	ru_minflt;	/* page reclaims */
	__kernel_long_t	ru_majflt;	/* page faults */
	__kernel_long_t	ru_nswap;	/* swaps */
	__kernel_long_t	ru_inblock;	/* block input operations */
	__kernel_long_t	ru_oublock;	/* block output operations */
	__kernel_long_t	ru_msgsnd;	/* messages sent */
	__kernel_long_t	ru_msgrcv;	/* messages received */
	__kernel_long_t	ru_nsignals;	/* signals received */
	__kernel_long_t	ru_nvcsw;	/* voluntary context switches */
	__kernel_long_t	ru_nivcsw;	/* involuntary " */
} rusage_t;


/* current running task PCB */
extern pcb_t *volatile current_running0;
extern pcb_t *volatile current_running1;
extern pcb_t **volatile current_running;
/* ready queue to run */
extern list_head ready_queue;
// pcb_t * volatile (*current_running)[NR_CPUS];
extern pid_t process_id;
extern int allocpid;

extern pcb_t pcb[NUM_MAX_TASK];
// pcb_t kernel_pcb[NR_CPUS];
extern const ptr_t pid0_stack;
extern const ptr_t pid0_stack2;
extern pcb_t pid0_pcb;
extern pcb_t pid0_pcb2;

extern int freepidnum;
extern int nowpid;
extern int glmask;
extern int glvalid;

extern pid_t freepid[NUM_MAX_TASK];

pid_t nextpid();

void init_pcb();
int check_pcb(int cpuid);

void switch_to(pcb_t *prev, pcb_t *next);
long k_scheduler(void);
long sys_sched_yield(void);
long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp);
void check_sleeping();

void k_block(list_head *, list_head *queue);
void k_unblock(list_head *, list_head *, int way);
long k_taskset(int pid, int mask);

long sys_spawn(task_info_t *task, void *arg, spawn_mode_t mode);
long sys_fork(int prior);
// long sys_execve(const char *filename, const char **argv, const char **envp);
// long sys_clone(int (*fn)(void *), void *stack, int flags, void *arg, pid_t *parent_tid, void *tls, pid_t *child_tid);
long sys_kill(pid_t pid);
long sys_exit(int error_code);
long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru);
long sys_process_show();
long sys_exec(const char *file_name, int argc, char **argv);
long sys_show_exec();
long sys_setpriority(int which, int who, int niceval);
long sys_getpriority(int which, int who);
long sys_getpid(void);
long sys_getppid(void);

long sys_mthread_create(pid_t *thread, void (*start_routine)(void *), void *arg);