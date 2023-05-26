#pragma once

#include <lib/list.h>

#define LOCK_NUM 32

typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef enum {
    UNGUARDED,
    GUARDED,
} guard_status_t;

typedef struct spin_lock {
    volatile lock_status_t flag;
} spin_lock_t;

typedef struct double_spin_lock {
    volatile lock_status_t flag;
    volatile guard_status_t guard;
} double_spin_lock_t;

typedef struct mutex_lock {
    int lock_id;
    int initialized;
    double_spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

typedef struct sleep_lock {
    bool locked;
    spin_lock_t lk;
    char *name;
    int pid;
} sleep_lock_t;

/* init lock */
// for kernel_lock
void k_spin_lock_init(spin_lock_t *lock);
int k_spin_lock_try_acquire(spin_lock_t *lock);
void k_spin_lock_acquire(spin_lock_t *lock);
void k_spin_lock_release(spin_lock_t *lock);
void k_schedule_with_spin_lock(spin_lock_t *lock);

long k_mutex_lock_op(int *key, int op);
long k_mutex_lock_init(int *key);
long k_mutex_lock_acquire(int key);
long k_mutex_lock_release(int key);
long k_mutex_lock_destroy(int *key);
long k_mutex_lock_trylock(int *key);

void k_sleep_lock_init(sleep_lock_t *lk);
void k_sleep_lock_aquire(sleep_lock_t *lk);
void k_sleep_lock_release(sleep_lock_t *lk);
int k_sleep_lock_hold(sleep_lock_t *lk);