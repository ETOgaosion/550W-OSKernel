#include <asm/atomic.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/smp.h>

int first_time = 1;
mutex_lock_t *locks[LOCK_NUM];

void k_spin_lock_init(spin_lock_t *lock) {
    lock->flag = UNLOCKED;
}

int k_spin_lock_try_acquire(spin_lock_t *lock) {
    return atomic_swap_d(LOCKED, (ptr_t)&lock->flag);
}

void k_spin_lock_acquire(spin_lock_t *lock) {
    while (k_spin_lock_try_acquire(lock) == LOCKED)
        ;
}

void k_spin_lock_release(spin_lock_t *lock) {
    lock->flag = UNLOCKED;
}

void k_spin_lock_with_owner_init(spin_lock_with_owner_t *lock) {
    lock->owner = -1;
    k_spin_lock_init(&lock->lock);
}

void k_spin_lock_with_owner_acquire(spin_lock_with_owner_t *lock) {
    if (lock->owner == k_smp_get_current_cpu_id()) {
        return;
    } else {
        while (lock->owner != -1) {
            if (lock->owner != -1) {
                break;
            }
        }
        k_spin_lock_acquire(&lock->lock);
        lock->owner = k_smp_get_current_cpu_id();
    }
}

void k_spin_lock_with_owner_release(spin_lock_with_owner_t *lock) {
    lock->owner = -1;
    k_spin_lock_release(&lock->lock);
}

void k_schedule_with_spin_lock(spin_lock_t *lock) {
    int locked = lock->flag;
    k_spin_lock_release(lock);
    k_pcb_scheduler(false, false);
    if (locked == LOCKED) {
        k_spin_lock_acquire(lock);
    }
}

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
            locks[i] = (mutex_lock_t *)k_mm_malloc(sizeof(mutex_lock_t));
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
        (*current_running)->lock_ids[(*current_running)->locksum++] = key + 1;
        return locks[key]->lock_id;
    } else {
        k_pcb_block(&(*current_running)->list, &locks[key]->block_queue, ENQUEUE_LIST);
        locks[key]->lock.guard = 0;
        k_pcb_scheduler(false, false);
        return -2;
    }
}

long k_mutex_lock_release(int key) {
    if (!locks[key]->initialized) {
        return -1;
    }
    if ((*current_running)->locksum) {
        (*current_running)->lock_ids[--(*current_running)->locksum] = 0;
    }
    while (atomic_cmpxchg(UNGUARDED, GUARDED, (ptr_t) & (locks[key]->lock.guard)) == GUARDED) {
        ;
    }
    if (list_is_empty(&locks[key]->block_queue)) {
        locks[key]->lock.flag = 0;
    } else {
#ifdef RBTREE
        k_pcb_unblock(locks[key]->block_queue.next, NULL, UNBLOCK_TO_LIST_STRATEGY);
#else
        k_pcb_unblock(locks[key]->block_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
#endif
    }
    locks[key]->lock.guard = 0;
    return locks[key]->lock_id;
}

long k_mutex_lock_destroy(int *key) {
    if (!locks[*key - 1]->initialized) {
        return -1;
    }
    while ((*current_running)->locksum) {
        (*current_running)->lock_ids[--(*current_running)->locksum] = 0;
    }
    k_bzero((void *)locks[*key - 1], sizeof(mutex_lock_t *));
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

void k_sleep_lock_init(sleep_lock_t *lk) {
    k_spin_lock_init(&lk->lk);
    lk->locked = false;
    lk->pid = 0;
}

void k_sleep_lock_acquire(sleep_lock_t *lk) {
    k_spin_lock_acquire(&lk->lk);
    while (lk->locked) {
        k_pcb_sleep(lk, &lk->lk);
    }
    lk->locked = TRUE;
    lk->pid = (*current_running)->pid;
    k_spin_lock_release(&lk->lk);
}

void k_sleep_lock_release(sleep_lock_t *lk) {
    k_spin_lock_acquire(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    k_pcb_wakeup(lk);
    k_spin_lock_release(&lk->lk);
}

int k_sleep_lock_hold(sleep_lock_t *lk) {
    int r;
    k_spin_lock_acquire(&lk->lk);
    r = lk->locked && (lk->pid == (*current_running)->pid);
    k_spin_lock_release(&lk->lk);
    return r;
}

futex_bucket_t futex_buckets[FUTEX_BUCKETS]; // HASH LIST
futex_node_t futex_node[MAX_FUTEX_NUM];
int futex_node_used[MAX_FUTEX_NUM] = {0};
struct robust_list_head *robust_list[NUM_MAX_TASK] = {0};

void k_futex_init() {
    for (int i = 0; i < FUTEX_BUCKETS; ++i) {
        init_list_head(&futex_buckets[i]);
    }
}

static int futex_hash(uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ul;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebul;
    x = x ^ (x >> 31);
    return x % FUTEX_BUCKETS;
}

static futex_node_t *get_node(u32 *val_addr, int create) {
    int key = futex_hash((uint64_t)val_addr);
    list_node_t *head = &futex_buckets[key];
    for (list_node_t *p = head->next; p != head; p = p->next) {
        futex_node_t *node = list_entry(p, futex_node_t, list);
        if (node->futex_key == (uint64_t)val_addr) {
            return node;
        }
    }
    if (create) {
        int i = 0;
        for (i = 0; i < MAX_FUTEX_NUM; i++) {
            if (futex_node_used[i] == 0) {
                break;
            }
        }
        futex_node_used[i] = (*current_running)->pid;
        futex_node_t *node = &futex_node[i];
        node->futex_key = (uint64_t)val_addr;
        init_list_head(&node->block_queue);
        list_add_tail(&node->list, &futex_buckets[key]);
        node->set_ts.tv_nsec = 0;
        node->set_ts.tv_sec = 0;
        node->add_ts.tv_nsec = 0;
        node->add_ts.tv_sec = 0;
        return node;
    }

    return NULL;
}

int k_futex_wait(u32 *val_addr, u32 val, const kernel_timespec_t *timeout) {
    futex_node_t *node = get_node(val_addr, 1);
    if (timeout && (timeout->tv_nsec != 0 || timeout->tv_sec != 0)) {
        node->set_ts.tv_sec = timeout->tv_sec;
        node->set_ts.tv_nsec = timeout->tv_nsec;
        k_time_get_nanotime((nanotime_val_t *)&node->add_ts);
    } else if (timeout == 0) {
        node->set_ts.tv_sec = 0;
        node->set_ts.tv_nsec = 0;
    }
    k_pcb_block(&(*current_running)->list, &node->block_queue, ENQUEUE_LIST);
    k_pcb_scheduler(false, false);
    return 0;
}

int k_futex_wakeup(u32 *val_addr, u32 num_wakeup) {
    futex_node_t *node = get_node(val_addr, 0);
    int ret = 0;
    if (node != NULL) {
        for (int i = 0; i < num_wakeup; ++i) {
            if (list_is_empty(&node->block_queue)) {
                break;
            }
            k_pcb_unblock(node->block_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        }
        ret++;
    }
    return ret;
}
int k_futex_requeue(u32 *uaddr, u32 *uaddr2, u32 num) {
    int ret = 0;
    futex_node_t *node1 = get_node(uaddr, 0);
    futex_node_t *node2 = get_node(uaddr2, 0);
    if (node1 == NULL) {
        node1 = get_node(uaddr, 1);
    }
    if (node2 == NULL) {
        node2 = get_node(uaddr2, 1);
    }
    for (int i = 0; i < num; ++i) {
        if (list_is_empty(&node1->block_queue)) {
            break;
        }
        list_head *pcb_node = node1->block_queue.next;
        list_del(pcb_node);
        list_add(pcb_node, &node2->block_queue);
        ret++;
    }
    return ret;
}

void check_futex_timeout() {
    kernel_timespec_t cur_time;
    k_time_get_nanotime((nanotime_val_t *)&cur_time);
    for (int i = 0; i < MAX_FUTEX_NUM; i++) {
        if (futex_node_used[i]) {
            if (futex_node[i].set_ts.tv_nsec == 0 && futex_node[i].set_ts.tv_sec == 0) {
                continue;
            }
            if ((cur_time.tv_sec - futex_node[i].add_ts.tv_sec) > futex_node[i].set_ts.tv_sec) {
                futex_node[i].set_ts.tv_sec = 0;
                futex_node[i].set_ts.tv_nsec = 0;
                futex_node[i].add_ts.tv_sec = 0;
                futex_node[i].add_ts.tv_nsec = 0;
                while (1) {
                    if (list_is_empty(&futex_node[i].block_queue)) {
                        break;
                    }
                    k_pcb_unblock(futex_node[i].block_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
                }
            } else if ((cur_time.tv_sec - futex_node[i].add_ts.tv_sec) == futex_node[i].set_ts.tv_sec) {
                if (futex_node[i].set_ts.tv_nsec && (cur_time.tv_nsec - futex_node[i].add_ts.tv_nsec) >= futex_node[i].set_ts.tv_nsec) {
                    futex_node[i].set_ts.tv_sec = 0;
                    futex_node[i].set_ts.tv_nsec = 0;
                    futex_node[i].add_ts.tv_sec = 0;
                    futex_node[i].add_ts.tv_nsec = 0;
                    while (1) {
                        if (list_is_empty(&futex_node[i].block_queue)) {
                            break;
                        }
                        k_pcb_unblock(futex_node[i].block_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
                    }
                }
            }
        }
    }
}

int find_index(int pid) {
    for (int i = 1; i < NUM_MAX_TASK; i++) {
        if (pid == pcb[i].pid) {
            return i;
        }
    }
    return -1;
}

long sys_get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr) {

    if (pid == 0) {
        int current_index = find_index((*current_running)->pid);
        *head_ptr = robust_list[current_index];
        return 0;
    } else {
        int index = find_index(pid);
        *head_ptr = robust_list[index];
        return 0;
    }
}

long sys_set_robust_list(struct robust_list_head *head, size_t len) {
    int current_index = find_index((*current_running)->pid);
    robust_list[current_index] = head;
    return 0;
}

long sys_futex(u32 *uaddr, int op, u32 val, const kernel_timespec_t *utime, u32 *uaddr2, u32 val3) {
    if ((op & FUTEX_WAKE) == FUTEX_WAKE) {
        return (long)k_futex_wakeup(uaddr, val);
    } else if ((op & FUTEX_WAIT) == FUTEX_WAIT) {
        if (*uaddr == val) {
            k_futex_wait(uaddr, val, utime);
        } else {
            return -EAGAIN;
        }
    }
    if ((op & FUTEX_REQUEUE) == FUTEX_REQUEUE) {
        return k_futex_requeue(uaddr, uaddr2, utime->tv_sec);
    }
    return 0;
}