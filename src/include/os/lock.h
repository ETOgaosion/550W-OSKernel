#pragma once

#include <lib/list.h>
#include <os/time.h>

#define LOCK_NUM 32

#define FUTEX_BUCKETS 100
#define MAX_FUTEX_NUM 60

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_FD 2
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_LOCK_PI 6
#define FUTEX_UNLOCK_PI 7
#define FUTEX_TRYLOCK_PI 8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10
#define FUTEX_WAIT_REQUEUE_PI 11
#define FUTEX_CMP_REQUEUE_PI 12

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_CLOCK_REALTIME 256
#define FUTEX_CMD_MASK ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#define FUTEX_REQUEUE_PRIVATE (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_OP_PRIVATE (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE (FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE (FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_TRYLOCK_PI_PRIVATE (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_BITSET_PRIVATE (FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_BITSET_PRIVATE (FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE (FUTEX_WAIT_REQUEUE_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE (FUTEX_CMP_REQUEUE_PI | FUTEX_PRIVATE_FLAG)

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

typedef struct spin_lock_with_owner {
    spin_lock_t lock;
    int owner;
} spin_lock_with_owner_t;

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
typedef uint64_t futex_key_t;

typedef struct futex_node {
    futex_key_t futex_key;
    list_node_t list;
    list_head block_queue;
    kernel_timespec_t add_ts;
    kernel_timespec_t set_ts;
} futex_node_t;

struct robust_list {
    struct robust_list *next;
};
struct robust_list_head {
    /*
     * The head of the list. Points back to itself if empty:
     */
    struct robust_list list;

    /*
     * This relative offset is set by user-space, it gives the kernel
     * the relative position of the futex field to examine. This way
     * we keep userspace flexible, to freely shape its data-structure,
     * without hardcoding any particular offset into the kernel:
     */
    long futex_offset;

    /*
     * The death of the thread may race with userspace setting
     * up a lock's links. So to handle this race, userspace first
     * sets this field to the address of the to-be-taken lock,
     * then does the lock acquire, and then adds itself to the
     * list, and then clears this field. Hence the kernel will
     * always have full knowledge of all locks that the thread
     * _might_ have taken. We check the owner TID in any case,
     * so only truly owned locks will be handled.
     */
    struct robust_list *list_op_pending;
};

typedef list_head futex_bucket_t;

extern futex_bucket_t futex_buckets[FUTEX_BUCKETS]; // HASH LIST
extern futex_node_t futex_node[MAX_FUTEX_NUM];
extern int futex_node_used[MAX_FUTEX_NUM];

/* init lock */
// for
void k_spin_lock_init(spin_lock_t *lock);
int k_spin_lock_try_acquire(spin_lock_t *lock);
void k_spin_lock_acquire(spin_lock_t *lock);
void k_spin_lock_release(spin_lock_t *lock);
void k_schedule_with_spin_lock(spin_lock_t *lock);

void k_spin_lock_with_owner_init(spin_lock_with_owner_t *lock);
void k_spin_lock_with_owner_acquire(spin_lock_with_owner_t *lock);
void k_spin_lock_with_owner_release(spin_lock_with_owner_t *lock);

long k_mutex_lock_op(int *key, int op);
long k_mutex_lock_init(int *key);
long k_mutex_lock_acquire(int key);
long k_mutex_lock_release(int key);
long k_mutex_lock_destroy(int *key);
long k_mutex_lock_trylock(int *key);

void k_sleep_lock_init(sleep_lock_t *lk);
void k_sleep_lock_acquire(sleep_lock_t *lk);
void k_sleep_lock_release(sleep_lock_t *lk);
int k_sleep_lock_hold(sleep_lock_t *lk);

void check_futex_timeout();
void k_futex_init();
int k_futex_wait(u32 *val_addr, u32 val, const kernel_timespec_t *timeout);
int k_futex_wakeup(u32 *val_addr, u32 num_wakeup);
int k_futex_requeue(u32 *uaddr, u32 *uaddr2, u32 num);
long sys_get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
long sys_set_robust_list(struct robust_list_head *head, size_t len);

long sys_futex(u32 *uaddr, int op, u32 val, const kernel_timespec_t *utime, u32 *uaddr2, u32 val3);