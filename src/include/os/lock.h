#pragma once
#define LOCKS_NUM 10
#define MAX_MBOX_LENGTH 100

#include <lib/list.h>

typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef struct spin_lock {
    volatile lock_status_t status;
} spin_lock_t;

typedef struct mutex_lock {
    spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

typedef struct semaphore {
    int count;
    list_head block_queue;
} semaphore_t;

typedef struct barrier {
    int count;
    int countinit;
    list_head block_queue;
} barrier_t;

typedef struct mbox {
    char msg[MAX_MBOX_LENGTH + 5];
    int len_now;
    int users;
    list_head send_queue;
    list_head recv_queue;
} mbox_t;

/* init lock */
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

long sys_mutex_lock_init(int userid);
long sys_mutex_lock_acquire(int id);
long sys_mutex_lock_release(int id);

long sys_semaphore_init(int *id, int val);
long sys_semaphore_up(int *id);
long sys_semaphore_down(int *id);
long sys_semaphore_destroy(int *id);

long sys_barrier_init(int *id, unsigned count);
long sys_barrier_wait(int *id);
long sys_barrier_destroy(int *id);

long sys_mbox_open(const char *name);
long sys_mbox_close(int);
long sys_mbox_send(int, char *, int);
long sys_mbox_recv(int, char *, int);
long sys_test_send(int, int);
long sys_test_recv(int, int);
