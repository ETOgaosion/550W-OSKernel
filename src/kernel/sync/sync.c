#include <lib/string.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/sync.h>

int sem_first_time = 1;
int cond_first_time = 1;
int barrier_first_time = 1;
int mbox_first_time = 1;

Semaphore_t *sem_list[COMM_NUM];
cond_t *cond_list[COMM_NUM];
barrier_t *barrier_list[COMM_NUM];
mbox_t *mbox_list[COMM_NUM];

static int find_free(int type) {
    int find = 0;
    switch (type) {
    case 0:
        for (int i = 0; i < COMM_NUM; i++) {
            if (!sem_list[i]->sem_info.initialized) {
                find = 1;
                return i;
            }
        }
        break;
    case 1:
        for (int i = 0; i < COMM_NUM; i++) {
            if (!cond_list[i]->cond_info.initialized) {
                find = 1;
                return i;
            }
        }
        break;
    case 2:
        for (int i = 0; i < COMM_NUM; i++) {
            if (!barrier_list[i]->barrier_info.initialized) {
                find = 1;
                return i;
            }
        }
        break;
    case 3:
        for (int i = 0; i < COMM_NUM; i++) {
            if (!mbox_list[i]->mailbox_info.initialized) {
                find = 1;
                return i;
            }
        }
        break;

    default:
        break;
    }
    if (!find) {
        return -1;
    }
    return 0;
}

int k_commop(void *key_id, void *arg, int op) {
    int *key = (int *)key_id;
    switch (op) {
    case 0:
        return k_semaphore_init(key, *(int *)arg);
        break;
    case 1:
        return k_semaphore_p(*key - 1);
        break;
    case 2:
        return k_semaphore_v(*key - 1);
        break;
    case 3:
        return k_semaphore_destroy(key);
        break;
    case 4:
        return k_barrier_init(key, *(int *)arg);
        break;
    case 5:
        return k_barrier_wait(*key - 1);
        break;
    case 6:
        return k_barrier_destroy(key);
        break;
    case 7:
        return k_mbox_open((char *)key_id);
    case 8:
        return k_mbox_close();
    case 9:
        return k_mbox_send(*key - 1, (mbox_arg_t *)arg);
    case 10:
        return k_mbox_recv(*key - 1, (mbox_arg_t *)arg);
    case 11:
        return k_mbox_try_send(*key - 1, (mbox_arg_t *)arg);
    case 12:
        return k_mbox_try_recv(*key - 1, (mbox_arg_t *)arg);

    default:
        return 0;
        break;
    }
}

int k_semaphore_init(int *key, int sem) {
    if (sem_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            sem_list[i] = (Semaphore_t *)kmalloc(sizeof(Semaphore_t));
            sem_list[i]->sem_info.initialized = 0;
        }
        sem_first_time = 0;
    }
    if (*key > 0) {
        return -2;
    }
    if (*key < 0) {
        return -3;
    }
    int sem_i = find_free(0);
    if (sem_i < 0) {
        return -1;
    }
    sem_list[sem_i]->sem_info.id = sem_i + 1;
    sem_list[sem_i]->sem_info.initialized = 1;
    sem_list[sem_i]->sem = sem;
    init_list_head(&sem_list[sem_i]->wait_queue);
    *key = sem_i + 1;
    return 0;
}

int k_semaphore_p(int key) {
    if (!sem_list[key]->sem_info.initialized) {
        return -1;
    }
    sem_list[key]->sem--;
    if (sem_list[key]->sem < 0) {
        k_block(&(*current_running)->list, &sem_list[key]->wait_queue);
        k_scheduler();
    }
    return 0;
}

int k_semaphore_v(int key) {
    if (!sem_list[key]->sem_info.initialized) {
        return -1;
    }
    sem_list[key]->sem++;
    if (sem_list[key]->sem <= 0 && !list_is_empty(&sem_list[key]->wait_queue)) {
        k_unblock(sem_list[key]->wait_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
    }
    return 0;
}

int k_semaphore_destroy(int *key) {
    if (!sem_list[*key - 1]->sem_info.initialized) {
        return -1;
    }
    memset(sem_list[*key - 1], 0, sizeof(sem_list[*key - 1]));
    *key = 0;
    return 0;
}

int k_cond_init(int *key) {
    if (cond_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            cond_list[i] = (cond_t *)kmalloc(sizeof(cond_t));
            cond_list[i]->cond_info.initialized = 0;
        }
        cond_first_time = 0;
    }
    if (*key > 0) {
        return -2;
    }
    if (*key < 0) {
        return -3;
    }
    int cond_i = find_free(1);
    if (cond_i < 0) {
        return -1;
    }
    cond_list[cond_i]->cond_info.id = cond_i + 1;
    cond_list[cond_i]->cond_info.initialized = 1;
    cond_list[cond_i]->num_wait = 0;
    init_list_head(&cond_list[cond_i]->wait_queue);
    *key = cond_i + 1;
    return 0;
}

int k_cond_wait(int key) {
    if (!cond_list[key]->cond_info.initialized) {
        return -1;
    }
    cond_list[key]->num_wait++;
    k_block(&(*current_running)->list, &cond_list[key]->wait_queue);
    k_scheduler();
    return 0;
}

int k_cond_signal(int key) {
    if (!cond_list[key]->cond_info.initialized) {
        return -1;
    }
    if (cond_list[key]->num_wait > 0) {
        k_unblock(cond_list[key]->wait_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        cond_list[key]->num_wait--;
    }
    return 0;
}

int k_cond_broadcast(int key) {
    if (!cond_list[key]->cond_info.initialized) {
        return -1;
    }
    while (cond_list[key]->num_wait > 0) {
        k_unblock(cond_list[key]->wait_queue.next, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        cond_list[key]->num_wait--;
    }
    return 0;
}

int k_cond_destroy(int *key) {
    if (!cond_list[*key - 1]->cond_info.initialized) {
        return -1;
    }
    memset(cond_list[*key - 1], 0, sizeof(cond_list[*key]));
    *key = 0;
    return 0;
}

int k_barrier_init(int *key, int total) {
    if (barrier_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            barrier_list[i] = (barrier_t *)kmalloc(sizeof(barrier_t));
            barrier_list[i]->barrier_info.initialized = 0;
        }
        barrier_first_time = 0;
    }
    if (*key > 0) {
        return -2;
    }
    if (*key < 0) {
        return -3;
    }
    int barrier_i = find_free(2);
    if (barrier_i < 0) {
        return -1;
    }
    barrier_list[barrier_i]->barrier_info.id = barrier_i + 1;
    barrier_list[barrier_i]->barrier_info.initialized = 1;
    barrier_list[barrier_i]->count = 0;
    barrier_list[barrier_i]->total = total;
    barrier_list[barrier_i]->cond_id = 0;
    k_cond_init(&barrier_list[barrier_i]->cond_id);
    *key = barrier_i + 1;
    return 0;
}

int k_barrier_wait(int key) {
    if (!barrier_list[key]->barrier_info.initialized) {
        return -1;
    }
    if ((++barrier_list[key]->count) == barrier_list[key]->total) {
        barrier_list[key]->count = 0;
        k_cond_broadcast(barrier_list[key]->cond_id - 1);
    } else {
        k_cond_wait(barrier_list[key]->cond_id - 1);
    }
    return 0;
}

int k_barrier_destroy(int *key) {
    if (!barrier_list[*key - 1]->barrier_info.initialized) {
        return -1;
    }
    k_cond_destroy(&barrier_list[*key - 1]->cond_id);
    memset(barrier_list[*key - 1], 0, sizeof(barrier_list[*key - 1]));
    *key = 0;
    return 0;
}

int k_mbox_open(char *name) {
    int operator= sys_getpid();
    if (mbox_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            mbox_list[i] = (mbox_t *)kmalloc(sizeof(mbox_t));
            mbox_list[i]->mailbox_info.initialized = 0;
        }
        mbox_first_time = 0;
    }
    if (name == NULL) {
        return -2;
    }
    for (int i = 0; i < COMM_NUM; i++) {
        if (mbox_list[i]->mailbox_info.initialized && strcmp(mbox_list[i]->name, name) == 0) {
            mbox_list[i]->cited_pid[mbox_list[i]->cited_num++] = operator;
            pcb[operator-1].mboxid[pcb[operator-1].mboxsum++] = i + 1;
            return i + 1;
        }
    }
    int mbox_i = find_free(3);
    if (mbox_i < 0) {
        return -1;
    }
    mbox_list[mbox_i]->mailbox_info.id = mbox_i + 1;
    mbox_list[mbox_i]->mailbox_info.initialized = 1;
    strcpy(mbox_list[mbox_i]->name, name);
    memset(mbox_list[mbox_i]->buff, 0, sizeof(mbox_list[mbox_i]->buff));
    mbox_list[mbox_i]->read_head = 0;
    mbox_list[mbox_i]->write_tail = 0;
    mbox_list[mbox_i]->used_units = 0;
    mbox_list[mbox_i]->full_cond_id = 0;
    mbox_list[mbox_i]->empty_cond_id = 0;
    k_cond_init(&mbox_list[mbox_i]->full_cond_id);
    k_cond_init(&mbox_list[mbox_i]->empty_cond_id);
    memset(mbox_list[mbox_i]->cited_pid, 0, sizeof(mbox_list[mbox_i]->cited_pid));
    mbox_list[mbox_i]->cited_num = 0;
    mbox_list[mbox_i]->cited_pid[mbox_list[mbox_i]->cited_num++] = operator;
    pcb[operator-1].mboxid[pcb[operator-1].mboxsum++] = mbox_i + 1;
    return mbox_i + 1;
}

int k_mbox_close() {
    int operator= sys_getpid();
    for(int i = 0; i < pcb[operator-1].mboxsum; i++){
        mbox_list[pcb[operator-1].mboxid[i] - 1]->cited_num--;
        if(mbox_list[pcb[operator-1].mboxid[i] - 1]->cited_num == 0){
            k_cond_destroy(&mbox_list[pcb[operator-1].mboxid[i] - 1]->full_cond_id);
            k_cond_destroy(&mbox_list[pcb[operator-1].mboxid[i] - 1]->empty_cond_id);
            memset(mbox_list[pcb[operator-1].mboxid[i] - 1],0,sizeof(mbox_list[pcb[operator-1].mboxid[i] - 1]));
        }
    }
    return 0;
}

int k_mbox_send(int key, mbox_arg_t *arg) {
    if (!mbox_list[key]->mailbox_info.initialized) {
        return -1;
    }
    if (arg->sleep_operation == 1 && k_mbox_try_send(key, arg) < 0) {
        return -2;
    }
    int blocked_time = 0;
    while (arg->msg_length > MBOX_MSG_MAX_LEN - mbox_list[key]->used_units) {
        blocked_time++;
        k_cond_wait(mbox_list[key]->full_cond_id - 1);
    }
    int left_space = MBOX_MSG_MAX_LEN - (mbox_list[key]->write_tail + arg->msg_length);
    if (left_space < 0) {
        memcpy((uint8_t *)mbox_list[key]->buff + mbox_list[key]->write_tail, arg->msg, MBOX_MSG_MAX_LEN - mbox_list[key]->write_tail);
        memcpy((uint8_t *)mbox_list[key]->buff, arg->msg + MBOX_MSG_MAX_LEN - mbox_list[key]->write_tail, -left_space);
        mbox_list[key]->write_tail = -left_space;
    } else {
        memcpy((uint8_t *)mbox_list[key]->buff + mbox_list[key]->write_tail, arg->msg, arg->msg_length);
        mbox_list[key]->write_tail += arg->msg_length;
    }
    mbox_list[key]->used_units += arg->msg_length;
    k_cond_broadcast(mbox_list[key]->empty_cond_id - 1);
    return blocked_time;
}

int k_mbox_recv(int key, mbox_arg_t *arg) {
    if (!mbox_list[key]->mailbox_info.initialized) {
        return -1;
    }
    if (arg->sleep_operation == 1 && k_mbox_try_recv(key, arg) < 0) {
        return -2;
    }
    int blocked_time = 0;
    while (arg->msg_length > mbox_list[key]->used_units) {
        blocked_time++;
        k_cond_wait(mbox_list[key]->empty_cond_id - 1);
    }
    int left_space = MBOX_MSG_MAX_LEN - (mbox_list[key]->read_head + arg->msg_length);
    if (left_space < 0) {
        memcpy((uint8_t *)arg->msg, (const uint8_t *)mbox_list[key]->buff + mbox_list[key]->read_head, MBOX_MSG_MAX_LEN - mbox_list[key]->read_head);
        memcpy((uint8_t *)arg->msg + MBOX_MSG_MAX_LEN - mbox_list[key]->read_head, (const uint8_t *)mbox_list[key]->buff, -left_space);
        mbox_list[key]->read_head = -left_space;
    } else {
        memcpy((uint8_t *)arg->msg, (const uint8_t *)mbox_list[key]->buff + mbox_list[key]->read_head, arg->msg_length);
        mbox_list[key]->read_head += arg->msg_length;
    }
    mbox_list[key]->used_units -= arg->msg_length;
    k_cond_broadcast(mbox_list[key]->full_cond_id - 1);
    return blocked_time;
}

int k_mbox_try_send(int key, mbox_arg_t *arg) {
    if (!mbox_list[key]->mailbox_info.initialized) {
        return -1;
    }
    if (arg->msg_length > MBOX_MSG_MAX_LEN - mbox_list[key]->used_units) {
        return -2;
    }
    return 0;
}

int k_mbox_try_recv(int key, mbox_arg_t *arg) {
    if (!mbox_list[key]->mailbox_info.initialized) {
        return -1;
    }
    if (arg->msg_length > mbox_list[key]->used_units) {
        return -2;
    }
    return 0;
}