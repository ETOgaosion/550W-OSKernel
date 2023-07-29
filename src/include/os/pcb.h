#pragma once

#include <asm/context.h>
#include <common/elf.h>
#include <lib/list.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/prctl.h>
#include <os/resources.h>
#include <os/signal.h>
#include <os/sync.h>
#include <os/time.h>

#define NUM_MAX_PCB_NAME 20
#define NUM_MAX_PROCESS 16
#define NUM_MAX_CHILD 5
#define NUM_MAX_CHILD_THREADS 5
#define NUM_MAX_TASK (1 + NUM_MAX_CHILD + NUM_MAX_CHILD_THREADS) * NUM_MAX_PROCESS
#define NUM_MAX_LOCK 16
#define NUM_MAX_MBOX 16

#define MAX_PRIORITY 5

#define STACK_SIZE 4 * PAGE_SIZE

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

/*
 * Scheduling policies
 */
#define SCHED_NORMAL 0
#define SCHED_OTHER 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE 5
#define SCHED_DEADLINE 6
/* I/O scheduler */

/* Can be ORed in to make sure the process is reverted back to SCHED_NORMAL on fork */
#define SCHED_RESET_ON_FORK 0x40000000

/*
 * Flags for bug emulation.
 *
 * These occupy the top three bytes.
 */
enum {
    UNAME26 = 0x0020000,
    ADDR_NO_RANDOMIZE = 0x0040000, /* disable randomization of VA space */
    FDPIC_FUNCPTRS = 0x0080000,    /* userspace function ptrs point to descriptors
                                    * (signal handling)
                                    */
    MMAP_PAGE_ZERO = 0x0100000,
    ADDR_COMPAT_LAYOUT = 0x0200000,
    READ_IMPLIES_EXEC = 0x0400000,
    ADDR_LIMIT_32BIT = 0x0800000,
    SHORT_INODE = 0x1000000,
    WHOLE_SECONDS = 0x2000000,
    STICKY_TIMEOUTS = 0x4000000,
    ADDR_LIMIT_3GB = 0x8000000,
};

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
    ENQUEUE_LIST_PRIORITY,
    ENQUEUE_TIMER_LIST,
} enqueue_way_t;

typedef enum dequeue_way {
    DEQUEUE_LIST,
    DEQUEUE_LIST_FIFO,
    DEQUEUE_LIST_PRIORITY,
} dequeue_way_t;

typedef enum unblock_way {
    UNBLOCK_TO_LIST_FRONT,
    UNBLOCK_TO_LIST_BACK,
    UNBLOCK_ONLY,
    UNBLOCK_TO_LIST_STRATEGY,
} unblock_way_t;

typedef struct __user_cap_header_struct {
    __u32 version;
    int pid;
} cap_user_header_t;

typedef struct __user_cap_data_struct {
    __u32 effective;
    __u32 permitted;
    __u32 inheritable;
} cap_user_data_t;

typedef struct sched_param {
    int sched_priority;
} sched_param_t;

extern pcb_mbox_t pcb_mbox[NUM_MAX_PROCESS];

typedef struct gids {
    gid_t rgid;
    gid_t egid;
    gid_t sgid;
} gids_t;

typedef struct uids {
    uid_t ruid;
    uid_t euid;
    uid_t suid;
} uids_t;

/* Process Control Block */
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

    rusage_t resources;

    cap_user_data_t cap[2];
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

void k_pcb_init();
extern void switch_to(pcb_t *prev, pcb_t *next);
long k_pcb_scheduler(bool voluntary);
void k_pcb_block(list_head *, list_head *queue, enqueue_way_t way);
void k_pcb_unblock(list_head *, list_head *, unblock_way_t way);
long k_pcb_getpid(void);
void k_pcb_sleep(void *chan, spin_lock_t *lk);
void k_pcb_wakeup(void *chan);
int k_pcb_count();

extern void k_signal_send_signal(int signum, pcb_t *pcb);

long spawn(const char *file_name);
long sys_spawn(const char *file_name);
long sys_fork(void);
long sys_exec(const char *file_name, const char *argv[], const char *envp[]);
long sys_execve(const char *file_name, const char *argv[], const char *envp[]);
long sys_clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid);
long sys_kill(pid_t pid);
long sys_exit(int error_code);
long sys_exit_group(int error_code);
long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru);

long sys_process_show();
long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp);

long sys_setpriority(int which, int who, int niceval);
long sys_getpriority(int which, int who);

long sys_sched_setparam(pid_t pid, sched_param_t *param);
long sys_sched_setscheduler(pid_t pid, int policy, sched_param_t *param);
long sys_sched_getscheduler(pid_t pid);
long sys_sched_getparam(pid_t pid, sched_param_t *param);
long sys_sched_setaffinity(pid_t pid, unsigned int len, const uint8_t *user_mask_ptr);
long sys_sched_getaffinity(pid_t pid, unsigned int len, uint8_t *user_mask_ptr);
long sys_sched_yield(void);
long sys_sched_get_priority_max(int policy);
long sys_sched_get_priority_min(int policy);

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

long sys_getrusage(int who, rusage_t *ru);
long sys_prlimit64(pid_t pid, unsigned int resource, const rlimit64_t *new_rlim, rlimit64_t *old_rlim);

long sys_personality(unsigned int personality);
long sys_unshare(unsigned long unshare_flags);
long sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);

long sys_capget(cap_user_header_t *header, cap_user_data_t *dataptr);
long sys_capset(cap_user_header_t *header, const cap_user_data_t *data);