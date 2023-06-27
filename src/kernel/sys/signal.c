#include <os/signal.h>

// [TODO]
long sys_rt_sigaction(int signum, const sigaction_t *act, sigaction_t *oldact, size_t sigsetsize) {
    return 0;
}

long sys_rt_sigprocmask(int how, sigset_t *set, sigset_t *oset, size_t sigsetsize) {
    return 0;
}