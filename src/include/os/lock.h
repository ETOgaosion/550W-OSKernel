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

typedef struct spin_lock{
    volatile lock_status_t flag;
} spin_lock_t;

typedef struct double_spin_lock
{
    volatile lock_status_t flag;
    volatile guard_status_t guard;
} double_spin_lock_t;

typedef struct mutex_lock
{
    int lock_id;
    int initialized;
    double_spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

/* init lock */
// for kernel_lock
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

long k_mutex_lock_op(int *key,int op);
long k_mutex_lock_init(int *key);
long k_mutex_lock_acquire(int key);
long k_mutex_lock_release(int key);
long k_mutex_lock_destroy(int *key);
long k_mutex_lock_trylock(int *key);
