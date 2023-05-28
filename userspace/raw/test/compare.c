#include <mthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#define MAX 100000
#define KEY 10
#define SHMP_KEY 5

uint64_t timebase;
uint64_t thread_data = 0;
uint64_t check_ans = 0;
int locked = 0;

typedef struct consensus_vars {
    int data;
    mthread_barrier_t barrier;
    int binsemid;
} consensus_vars_t;

void lock(int *l) {
    while (atomic_exchange(l, 1))
        ;
}

void unlock(int *l) {
    atomic_exchange(l, 0);
}

void add(int m) {
    for (int i = m; i < MAX; i += 2) {
        lock(&locked);
        thread_data += i;
        unlock(&locked);
    }
    sys_exit();
}

int main(int argc, char *argv[]) {
    consensus_vars_t *vars = (consensus_vars_t *)sys_shmpageget(SHMP_KEY);
    if (argc == 1) {
        for (int i = 0; i < MAX; i++) {
            check_ans += i;
        }
        sys_move_cursor(1, 1);
        printf("check ans: %d\n", check_ans);

        timebase = sys_get_timebase();
        vars->data = 0;
        // sys_move_cursor(1, 6+argc);
        // printf("%x, %x\n", sys_getpa(vars), sys_getpa(&(vars->barrier)));
        mthread_barrier_init(&(vars->barrier), 2);
        vars->binsemid = binsemget(KEY);

        char *subargv[2];
        char number[4];
        char name[10] = "compare";
        number[0] = '1';
        number[1] = 0;
        subargv[0] = name;
        subargv[1] = number;
        // start new process
        sys_exec(subargv[0], 2, subargv);

        sys_move_cursor(1, 2);
        printf("two PROCESS start ...\n");
        mthread_barrier_wait(&(vars->barrier));
        uint64_t time_pro = 0;
        time_pro = sys_get_tick() / timebase;
        for (int i = 1; i < MAX; i += 2) {
            binsemop(vars->binsemid, 0);
            vars->data += i;
            binsemop(vars->binsemid, 1);
        }
        mthread_barrier_wait(&(vars->barrier));

        time_pro = (sys_get_tick() / timebase) - time_pro;
        sys_move_cursor(1, 3);
        printf("[PROCESS] ans: %d, cost: %d s\n", vars->data, time_pro);

        // thread test
        printf("two THREAD start ...\n");
        mthread_t id1, id2;
        uint64_t time_thread;
        time_thread = sys_get_tick() / timebase;
        mthread_create(&id1, add, 0);
        mthread_create(&id2, add, 1);
        mthread_join(id1);
        mthread_join(id2);
        time_thread = (sys_get_tick() / timebase) - time_thread;
        printf("[THREAD ] ans: %d, cost: %d s\n", thread_data, time_thread);
        uint64_t prop = (time_pro * 100) / time_thread;
        printf("[PROP] %d.%d\n", prop / 100, prop % 100);
    } else {
        vars->data = 0;
        mthread_barrier_wait(&(vars->barrier));
        for (int i = 0; i < MAX; i += 2) {
            binsemop(vars->binsemid, 0);
            vars->data += i;
            binsemop(vars->binsemid, 1);
        }
        mthread_barrier_wait(&(vars->barrier));
    }
    return 0;
}
