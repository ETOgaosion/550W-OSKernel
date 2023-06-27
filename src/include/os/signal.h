#pragma once

#include <asm/common.h>
#include <common/types.h>

#define _NSIG 64
#define _NSIG_BPW __BITS_PER_LONG
#define _NSIG_WORDS (_NSIG / _NSIG_BPW)

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

#define MINSIGSTKSZ 2048
#define SIGSTKSZ 8192

typedef void __signalfn_t(int);
typedef __signalfn_t *__sighandler_t;

typedef void __restorefn_t(void);
typedef __restorefn_t *__sigrestore_t;

typedef struct {
    unsigned long sig[_NSIG_WORDS];
} sigset_t;

typedef struct sigaction {
    __sighandler_t sa_handler;
    unsigned long sa_flags;
    __sigrestore_t sa_restorer;
    sigset_t sa_mask; /* mask last for extensibility */
} sigaction_t;

typedef struct sigaltstack {
    void *ss_sp;
    int ss_flags;
    __kernel_size_t ss_size;
} stack_t;

long sys_rt_sigaction(int signum, const sigaction_t *act, sigaction_t *oldact, size_t sigsetsize);
long sys_rt_sigprocmask(int how, sigset_t *set, sigset_t *oset, size_t sigsetsize);