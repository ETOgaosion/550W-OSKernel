#include <asm/atomic.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/pcb.h>

int first_time = 1;
mutex_lock_t *locks[LOCK_NUM];

void spin_lock_init(spin_lock_t *lock) { lock->flag = UNLOCKED; }

int spin_lock_try_acquire(spin_lock_t *lock) { return atomic_swap_d(LOCKED, (ptr_t)&lock->flag); }

void spin_lock_acquire(spin_lock_t *lock) {
    while (spin_lock_try_acquire(lock) == LOCKED)
        ;
}

void spin_lock_release(spin_lock_t *lock) { lock->flag = UNLOCKED; }

long k_mutex_lock_op(int *key, int op) {
    if (op == 0) {
        return k_mutex_lock_init(key);
    } else if (op == 1) {
        return k_mutex_lock_acquire(*key - 1);
    } else if (op == 2) {
        return k_mutex_lock_release(*key - 1);
    } else if (op == 3) {
        return k_mutex_lock_destroy(key);
    } else if (op == 4) {
        return k_mutex_lock_trylock(key);
    }
    return -1;
}

static inline int find_lock() {
    for (int i = 0; i < LOCK_NUM; i++) {
        if (!locks[i]->initialized) {
            return i;
        }
    }
    return -1;
}

long k_mutex_lock_init(int *key) {
    if (first_time) {
        for (int i = 0; i < LOCK_NUM; i++) {
            locks[i] = (mutex_lock_t *)kmalloc(sizeof(mutex_lock_t));
            locks[i]->initialized = 0;
        }
        first_time = 0;
    }
    if (*key > 0) {
        return -2;
    }
    if (*key < 0) {
        return -3;
    }
    int lock_i = find_lock();
    if (lock_i < 0) {
        return -1;
    }
    locks[lock_i]->lock_id = lock_i + 1;
    locks[lock_i]->initialized = 1;
    locks[lock_i]->lock.flag = UNLOCKED;
    locks[lock_i]->lock.guard = UNGUARDED;
    init_list_head(&(locks[lock_i]->block_queue));
    *key = lock_i + 1;
    return 0;
}

long k_mutex_lock_acquire(int key) {
    if (!locks[key]->initialized) {
        return -1;
    }
    while (atomic_cmpxchg(UNGUARDED, GUARDED, (ptr_t) & (locks[key]->lock.guard)) == GUARDED) {
        ;
    }
    if (locks[key]->lock.flag == 0) {
        locks[key]->lock.flag = 1;
        locks[key]->lock.guard = 0;
        (*current_running)->lockid[(*current_running)->locksum++] = key + 1;
        return locks[key]->lock_id;
    } else {
        k_block(&(*current_running)->list, &locks[key]->block_queue);
        locks[key]->lock.guard = 0;
        k_scheduler();
        return -2;
    }
}

long k_mutex_lock_release(int key) {
    if (!locks[key]->initialized) {
        return -1;
    }
    if ((*current_running)->locksum) {
        (*current_running)->lockid[--(*current_running)->locksum] = 0;
    }
    while (atomic_cmpxchg(UNGUARDED, GUARDED, (ptr_t) & (locks[key]->lock.guard)) == GUARDED) {
        ;
    }
    if (list_is_empty(&locks[key]->block_queue)) {
        locks[key]->lock.flag = 0;
    } else {
        k_unblock(locks[key]->block_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
    }
    locks[key]->lock.guard = 0;
    return locks[key]->lock_id;
}

long k_mutex_lock_destroy(int *key) {
    if (!locks[*key - 1]->initialized) {
        return -1;
    }
    while ((*current_running)->locksum) {
        (*current_running)->lockid[--(*current_running)->locksum] = 0;
    }
    memset(locks[*key - 1], 0, sizeof(locks[*key]));
    *key = 0;
    return 0;
}

long k_mutex_lock_trylock(int *key) {
    if (*key > 0) {
        return -2;
    }
    if (*key < 0) {
        return -3;
    }
    int lock_i = find_lock();
    if (lock_i < 0) {
        return -1;
    }
    return 0;
}