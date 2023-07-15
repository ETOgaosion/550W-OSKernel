#include <fs/fat32.h>
#include <fs/file.h>
#include <os/mm.h>
#include <os/msg.h>
#include <os/pcb.h>
#include <os/resources.h>
#include <os/signal.h>

all_resources_t moss_resources;

void k_set_rlimit(rlimit_t *target, kernel_ulong_t cur, kernel_ulong_t max) {
    target->rlim_cur = cur;
    if (max > 0) {
        target->rlim_max = max;
    }
}

void k_resources_init() {
    k_set_rlimit(&moss_resources[RLIMIT_CPU], CPU_NUM, CPU_NUM);
    k_set_rlimit(&moss_resources[RLIMIT_FSIZE], MAX_FILE_SIZE, MAX_FILE_SIZE);
    k_set_rlimit(&moss_resources[RLIMIT_DATA], GB, GB);
    k_set_rlimit(&moss_resources[RLIMIT_STACK], STACK_SIZE, STACK_SIZE);
    k_set_rlimit(&moss_resources[RLIMIT_CORE], MAX_FILE_SIZE, MAX_FILE_SIZE);
    k_set_rlimit(&moss_resources[RLIMIT_RSS], MEM_SIZE * PAGE_SIZE, MEM_SIZE * PAGE_SIZE);
    k_set_rlimit(&moss_resources[RLIMIT_NPROC], NUM_MAX_TASK, NUM_MAX_TASK);
    k_set_rlimit(&moss_resources[RLIMIT_NOFILE], MAX_FD, MAX_FD);
    k_set_rlimit(&moss_resources[RLIMIT_MEMLOCK], RLIM_INFINITY, RLIM_INFINITY);
    k_set_rlimit(&moss_resources[RLIMIT_AS], RLIM_INFINITY, RLIM_INFINITY);
    k_set_rlimit(&moss_resources[RLIMIT_LOCKS], RLIM_INFINITY, RLIM_INFINITY);
    k_set_rlimit(&moss_resources[RLIMIT_SIGPENDING], NUM_MAX_SIGPENDING, NUM_MAX_MQ);
    k_set_rlimit(&moss_resources[RLIMIT_MSGQUEUE], NUM_MAX_MQ, NUM_MAX_MQ);
    k_set_rlimit(&moss_resources[RLIMIT_NICE], RLIM_INFINITY, RLIM_INFINITY);
    k_set_rlimit(&moss_resources[RLIMIT_RTPRIO], MAX_PRIORITY, MAX_PRIORITY);
    k_set_rlimit(&moss_resources[RLIMIT_RTTIME], RLIM_INFINITY, RLIM_INFINITY);
}

long sys_getrlimit(unsigned int resource, rlimit_t *rlim) {
    if (resource < 0 || resource >= RLIM_NLIMITS) {
        return -EINVAL;
    }
    k_memcpy((uint8_t *)rlim, (const uint8_t *)&moss_resources[resource], sizeof(rlimit_t));
    return 0;
}

long sys_setrlimit(unsigned int resource, rlimit_t *rlim) {
    if (resource < 0 || resource >= RLIM_NLIMITS) {
        return -EINVAL;
    }
    k_memcpy((uint8_t *)&moss_resources[resource], (const uint8_t *)rlim, sizeof(rlimit_t));
    return 0;
}