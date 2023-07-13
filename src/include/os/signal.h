#pragma once

#include <asm/common.h>
#include <asm/context.h>
#include <common/types.h>

#define _NSIG 64
#define _NSIG_BPW __BITS_PER_LONG
#define _NSIG_WORDS (_NSIG / _NSIG_BPW)

#define NUM_SIG 34

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPOLL SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR 30
#define SIGSYS 31
#define SIGUNUSED 31

/* These should not be considered constants from userland.  */
#define SIGRTMIN 32
#ifndef SIGRTMAX
#define SIGRTMAX _NSIG
#endif

#define SIGTIMER 32
#define SIGCANCEL 33
#define SIGSYNCCALL 34

#define MINSIGSTKSZ 2048
#define SIGSTKSZ 8192

#define SA_RESTART 0x10000000
#define SA_RESTORER 0x04000000

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)

/* for sigprocmask */
#define SIG_BLOCK 0x0
#define SIG_UNBLOCK 0x1
#define SIG_SETMASK 0x2
#define NUM_MAX_SIGNAL 64

#define SIGNAL_HANDLER_ADDR 0xf10010000lu

#define ISSET_SIG(signum, pcb) ((pcb)->sig_recv & (1lu << ((signum)-1)))

#define CLEAR_SIG(signum, pcb) ((pcb)->sig_recv &= ~(1lu << ((signum)-1)))

#define SET_SIG(signum, pcb) ((pcb)->sig_recv |= (1lu << ((signum)-1)))

typedef void __signalfn_t(int);
typedef __signalfn_t *__sighandler_t;

typedef void __restorefn_t(void);
typedef __restorefn_t *__sigrestore_t;

typedef struct sigset {
    unsigned long sig[_NSIG_WORDS];
} sigset_t;

typedef struct siginfo {
    int si_signo;         /* Signal number */
    int si_errno;         /* An errno value */
    int si_code;          /* Signal code */
    int si_trapno;        /* Trap number that caused
                            hardware-generated signal
                            (unused on most architectures) */
    pid_t si_pid;         /* Sending process ID */
    uid_t si_uid;         /* Real user ID of sending process */
    int si_status;        /* Exit value or signal */
    clock_t si_utime;     /* User time consumed */
    clock_t si_stime;     /* System time consumed */
    sigval_t si_value;    /* Signal value */
    int si_int;           /* POSIX.1b signal */
    void *si_ptr;         /* POSIX.1b signal */
    int si_overrun;       /* Timer overrun count;
                            POSIX.1b timers */
    int si_timerid;       /* Timer ID; POSIX.1b timers */
    void *si_addr;        /* Memory location which caused fault */
    long si_band;         /* Band event (was int in glibc 2.3.2 and earlier) */
    int si_fd;            /* File descriptor */
    short si_addr_lsb;    /* Least significant bit of address
                            (since Linux 2.6.32) */
    void *si_lower;       /* Lower bound when address violation
                            occurred (since Linux 3.19) */
    void *si_upper;       /* Upper bound when address violation */
    int si_pkey;          /* Protection key on PTE that caused
                            fault (since Linux 4.6) */
    void *si_call_addr;   /* Address of system call instruction
                            (since Linux 3.5) */
    int si_syscall;       /* Number of attempted system call
                            (since Linux 3.5) */
    unsigned int si_arch; /* Architecture of attempted system call
                            (since Linux 3.5) */
} siginfo_t;

typedef struct sigaction {
    __sighandler_t sa_handler;
    unsigned long sa_flags;
    __sigrestore_t sa_restorer;
    sigset_t sa_mask; /* mask last for extensibility */
} sigaction_t;

typedef struct sigaltstack {
    void *ss_sp;
    int ss_flags;
    kernel_size_t ss_size;
} stack_t;

typedef struct sigset_t {
    unsigned long __bits[128 / sizeof(long)];
} usigset_t;

typedef struct ucontext {
    unsigned long uc_flags;
    struct ucontext_t *uc_link;
    stack_t uc_stack;
    usigset_t uc_sigmask;
    asm_mcontext_t uc_mcontext;
} ucontext_t;

typedef struct sigaction_table {
    int used;
    int num; // the number of process;
    sigaction_t sigactions[NUM_SIG];
} sigaction_table_t;

extern void enter_signal_trampoline();
extern void exit_signal_trampoline();

void k_signal_handler();

long sys_tkill(pid_t pid, int sig);
long sys_tgkill(pid_t tgid, pid_t pid, int sig);
long sys_rt_sigaction(int signum, const sigaction_t *act, sigaction_t *oldact, size_t sigsetsize);
long sys_rt_sigprocmask(int how, sigset_t *set, sigset_t *oset, size_t sigsetsize);
long sys_rt_sigreturn();