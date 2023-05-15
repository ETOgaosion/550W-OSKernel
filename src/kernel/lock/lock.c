#include <asm/atomic.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/smp.h>

mutex_lock_t locks[LOCKS_NUM];
semaphore_t semas[LOCKS_NUM];
int semaphore_id[LOCKS_NUM];
int semaphore_id_free = 0;
int semaphore_id_new = 0;
int lock_id_new = 0;
// int lock_id_free = 0;
// int lock_freeid[LOCKS_NUM];
int lock_userid[LOCKS_NUM];
barrier_t barris[LOCKS_NUM];
int barrier_id[LOCKS_NUM];
int barrier_id_free = 0;
int barrier_id_new = 0;
char mbox_name[LOCKS_NUM][MAX_MBOX_LENGTH];
mbox_t boxs[LOCKS_NUM];
int box_freeid[LOCKS_NUM];
int box_haveid[LOCKS_NUM];
int box_id_free = 0;
int box_id_have = 0;
int box_id_new = 0;

void spin_lock_init(spin_lock_t *lock) { lock->status = UNLOCKED; }

int spin_lock_try_acquire(spin_lock_t *lock) { return atomic_swap_d(LOCKED, (ptr_t)&lock->status); }

void spin_lock_acquire(spin_lock_t *lock) {
    while (spin_lock_try_acquire(lock) == LOCKED)
        ;
}

void spin_lock_release(spin_lock_t *lock) { lock->status = UNLOCKED; }

int get_new_box_id(const char *name) {
    int id = 0;
    if (box_id_free != 0)
        id = box_freeid[box_id_free--];
    else
        id = box_id_new++;
    kstrcpy(mbox_name[id], name);
    return id;
}

int do_mbox_open(const char *name) {
    int id = -1;
    for (int i = 1; i <= box_id_have; i++) {
        if (strcmp(name, mbox_name[box_haveid[i]]) == 0) {
            id = box_haveid[i];
            mbox_t *box = &boxs[id];
            box->users++;
            break;
        }
    }
    if (id == -1) {
        id = get_new_box_id(name);
        mbox_t *box = &boxs[id];
        box->len_now = 0;
        box->msg[0] = '\0';
        box->users = 1;
        box->send_queue.next = &box->send_queue;
        box->send_queue.prev = &box->send_queue;
        box->recv_queue.prev = &box->recv_queue;
        box->recv_queue.next = &box->recv_queue;
        box_id_have++;
        box_haveid[box_id_have] = id;
    }
    return id;
}

void do_mbox_close(int id) {
    mbox_t *box = &boxs[id];
    box->users--;
    if (box->users == 0) {
        box->msg[0] = '\0';
        box->len_now = 0;
        box_id_free++;
        box_freeid[box_id_free] = id;
        for (int i = 1; i <= box_id_have; i++) {
            if (box_haveid[i] == id) {
                for (int j = i; j < box_id_have; j++) {
                    box_haveid[j] = box_haveid[j + 1];
                }
            }
        }
        box_id_have--;
        box->send_queue.next = &box->send_queue;
        box->send_queue.prev = &box->send_queue;
        box->recv_queue.prev = &box->recv_queue;
        box->recv_queue.next = &box->recv_queue;
    }
}

int do_test_send(int id, int len) {
    mbox_t *box = &boxs[id];
    if (len + (box->len_now) > MAX_MBOX_LENGTH)
        return 0;
    else
        return 1;
}

int do_test_recv(int id, int len) {
    mbox_t *box = &boxs[id];
    if (len > box->len_now)
        return 0;
    else
        return 1;
}

int do_mbox_send(int id, char *msg, int len) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int blocktime = 0;
    mbox_t *box = &boxs[id];
    while (len + (box->len_now) > MAX_MBOX_LENGTH) {
        blocktime++;
        do_block(&(current_running->list), &(box->send_queue));
        do_scheduler();
    }
    while (box->recv_queue.next != &box->recv_queue)
        do_unblock(&(box->recv_queue), UNBLOCK_FROM_QUEUE);
    for (int i = 0; i < len; i++) {
        box->msg[i + box->len_now] = msg[i];
    }
    box->len_now += len;
    return blocktime;
}

int do_mbox_recv(int id, char *msg, int len) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int blocktime = 0;
    mbox_t *box = &boxs[id];
    while (len > box->len_now) {
        blocktime++;
        do_block(&(current_running->list), &(box->recv_queue));
        do_scheduler();
    }
    while (box->send_queue.next != &box->send_queue)
        do_unblock(&(box->send_queue), UNBLOCK_FROM_QUEUE);
    for (int i = 0; i < len; i++) {
        msg[i] = box->msg[i];
    }
    for (int i = len; i < box->len_now; i++) {
        box->msg[i - len] = box->msg[i];
    }
    box->len_now -= len;
    return blocktime;
}

int get_new_barrier_id() {
    if (barrier_id_free != 0)
        return barrier_id[barrier_id_free--];
    else
        return barrier_id_new++;
}

int do_barrier_init(int *id, unsigned count) {
    *id = get_new_barrier_id();
    if (*id > LOCKS_NUM)
        return 0;
    barrier_t *barri = &barris[*id];
    barri->countinit = count;
    barri->count = barri->countinit;
    barri->block_queue.next = &barri->block_queue;
    barri->block_queue.prev = &barri->block_queue;
    return 1;
}

int do_barrier_wait(int *id) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    barrier_t *barri = &barris[*id];
    barri->count--;
    if (barri->count == 0) {
        barri->count = barri->countinit;
        while (barri->block_queue.next != &barri->block_queue)
            do_unblock(&(barri->block_queue), UNBLOCK_FROM_QUEUE);
    } else if (barri->count > 0) {
        do_block(&(current_running->list), &(barri->block_queue));
        do_scheduler();
    }
    return 1;
}

int do_barrier_destroy(int *id) {
    barrier_t *barri = &barris[*id];
    barri->count = 0;
    barri->countinit = 0;
    barri->block_queue.next = &barri->block_queue;
    barri->block_queue.prev = &barri->block_queue;
    barrier_id_free++;
    barrier_id[semaphore_id_free] = *id;
    return 1;
}

int get_new_semaphore_id() {
    if (semaphore_id_free != 0)
        return semaphore_id[semaphore_id_free--];
    else
        return semaphore_id_new++;
}

int do_semaphore_init(int *id, int val) {
    *id = get_new_semaphore_id();
    if (*id > LOCKS_NUM)
        return 0;
    semaphore_t *sema = &semas[*id];
    sema->count = val;
    sema->block_queue.next = &sema->block_queue;
    sema->block_queue.prev = &sema->block_queue;
    return 1;
}

int do_semaphore_up(int *id) {
    semaphore_t *sema = &semas[*id];
    if (sema->count <= 0)
        do_unblock(&(sema->block_queue), UNBLOCK_FROM_QUEUE);
    sema->count++;
    return 1;
}

int do_semaphore_down(int *id) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    semaphore_t *sema = &semas[*id];
    while (sema->count <= 0) {
        do_block(&(current_running->list), &(sema->block_queue));
        do_scheduler();
    }
    sema->count--;
    return 1;
}

int do_semaphore_destroy(int *id) {
    semaphore_t *sema = &semas[*id];
    sema->count = 0;
    sema->block_queue.next = &sema->block_queue;
    sema->block_queue.prev = &sema->block_queue;
    semaphore_id_free++;
    semaphore_id[semaphore_id_free] = *id;
    return 0;
}

int do_mutex_lock_init(int userid) {
    /* TODO */
    int id = 0;
    if (lock_id_new >= LOCKS_NUM)
        assert(0);
    for (int i = 0; i < lock_id_new; i++) {
        if (lock_userid[i] == userid)
            return i;
    }
    // if(lock_id_free!=0)
    //    id = lock_freeid[lock_id_free--];
    // else
    id = lock_id_new++;
    lock_userid[id] = userid;
    mutex_lock_t *lock = &locks[id];
    lock->lock.status = UNLOCKED;
    lock->block_queue.next = &lock->block_queue;
    lock->block_queue.prev = &lock->block_queue;
    return id;
    /**id = lock_id_new++;
    if(lock_id_new>LOCKS_NUM) assert(0);
    mutex_lock_t *lock = &locks[*id];
    lock->lock.status=UNLOCKED;
    lock->block_queue.next=&lock->block_queue;
    lock->block_queue.prev=&lock->block_queue;*/
}

void do_mutex_lock_acquire(int id) {
    /* TODO */
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    mutex_lock_t *lock = &locks[id];
    while (lock->lock.status == LOCKED) {
        do_block(&(current_running->list), &(lock->block_queue));
        do_scheduler();
    }
    lock->lock.status = LOCKED;
    current_running->lockid[current_running->locksum] = id;
    current_running->locksum++;
}

void do_mutex_lock_release(int id) {
    /* TODO */
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    mutex_lock_t *lock = &locks[id];
    lock->lock.status = UNLOCKED;
    if (current_running->pid != 0)
        current_running->locksum--;
    do_unblock(&(lock->block_queue), UNBLOCK_FROM_QUEUE);
    // do_scheduler();
}
