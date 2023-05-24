#include <lib/math.h>
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
        return k_mbox_close((char *)key_id);
    case 9:
        return k_mbox_send(*key - 1, NULL, (mbox_arg_t *)arg);
    case 10:
        return k_mbox_recv(*key - 1, NULL, (mbox_arg_t *)arg);
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
            sem_list[i] = (Semaphore_t *)k_malloc(sizeof(Semaphore_t));
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
        k_block(&(*current_running)->list, &sem_list[key]->wait_queue, ENQUEUE_LIST);
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
    k_memset(sem_list[*key - 1], 0, sizeof(Semaphore_t *));
    *key = 0;
    return 0;
}

int k_cond_init(int *key) {
    if (cond_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            cond_list[i] = (cond_t *)k_malloc(sizeof(cond_t));
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
    k_block(&(*current_running)->list, &cond_list[key]->wait_queue, ENQUEUE_LIST);
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
    k_memset(cond_list[*key - 1], 0, sizeof(cond_t *));
    *key = 0;
    return 0;
}

int k_barrier_init(int *key, int total) {
    if (barrier_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            barrier_list[i] = (barrier_t *)k_malloc(sizeof(barrier_t));
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
    k_memset(barrier_list[*key - 1], 0, sizeof(barrier_t *));
    *key = 0;
    return 0;
}

int k_mbox_open(char *name) {
    int operator= sys_getpid();
    if (mbox_first_time) {
        for (int i = 0; i < COMM_NUM; i++) {
            mbox_list[i] = (mbox_t *)k_malloc(sizeof(mbox_t));
            mbox_list[i]->mailbox_info.initialized = 0;
        }
        mbox_first_time = 0;
    }
    if (name == NULL) {
        return -2;
    }
    for (int i = 0; i < COMM_NUM; i++) {
        if (mbox_list[i]->mailbox_info.initialized && k_strcmp(mbox_list[i]->name, name) == 0) {
            mbox_list[i]->cited_pid[mbox_list[i]->cited_num++] = operator;
            return i + 1;
        }
    }
    int mbox_i = find_free(3);
    if (mbox_i < 0) {
        return -1;
    }
    mbox_list[mbox_i]->mailbox_info.id = mbox_i + 1;
    mbox_list[mbox_i]->mailbox_info.initialized = 1;
    k_strcpy(mbox_list[mbox_i]->name, name);
    // k_memset(mbox_list[mbox_i]->buff, 0, sizeof(mbox_list[mbox_i]->buff));
    // mbox_list[mbox_i]->read_head = 0;
    // mbox_list[mbox_i]->write_tail = 0;
    // mbox_list[mbox_i]->used_units = 0;
    // mbox_list[mbox_i]->full_cond_id = 0;
    // mbox_list[mbox_i]->empty_cond_id = 0;
    k_cond_init(&mbox_list[mbox_i]->full_cond_id);
    k_cond_init(&mbox_list[mbox_i]->empty_cond_id);
    // k_memset(mbox_list[mbox_i]->cited_pid, 0, sizeof(mbox_list[mbox_i]->cited_pid));
    // mbox_list[mbox_i]->cited_num = 0;
    mbox_list[mbox_i]->cited_pid[mbox_list[mbox_i]->cited_num++] = operator;
    return mbox_i + 1;
}

int k_mbox_close(int mbox_id) {
    int operator= sys_getpid();
    mbox_t *target = mbox_list[mbox_id];
    for (int i = 0; i < target->cited_num; i++) {
        if (target->cited_pid[i] == operator) {
            target->cited_pid[i] = 0;
        }
    }
    if (!target->cited_num) {
        k_cond_destroy(&target->full_cond_id);
        k_cond_destroy(&target->empty_cond_id);
        k_memset((void *)target, 0, sizeof(mbox_t));
    }
    return 0;
}

int k_mbox_send(int key, mbox_t *target, mbox_arg_t *arg) {
    if (!target) {
        target = mbox_list[key];
    }
    if (!target->mailbox_info.initialized) {
        return -1;
    }
    if (arg->sleep_operation == 1 && k_mbox_try_send(key, arg) < 0) {
        return -2;
    }
    int blocked_time = 0;
    while (arg->msg_length > MBOX_MSG_MAX_LEN - target->used_units) {
        blocked_time++;
        k_cond_wait(target->full_cond_id - 1);
    }
    int left_space = MBOX_MSG_MAX_LEN - (target->write_tail + arg->msg_length);
    if (left_space < 0) {
        k_memcpy((uint8_t *)target->buff + target->write_tail, arg->msg, MBOX_MSG_MAX_LEN - target->write_tail);
        k_memcpy((uint8_t *)target->buff, arg->msg + MBOX_MSG_MAX_LEN - target->write_tail, -left_space);
        target->write_tail = -left_space;
    } else {
        k_memcpy((uint8_t *)target->buff + target->write_tail, arg->msg, arg->msg_length);
        target->write_tail += arg->msg_length;
    }
    target->used_units += arg->msg_length;
    k_cond_broadcast(target->empty_cond_id - 1);
    return blocked_time;
}

int k_mbox_recv(int key, mbox_t *target, mbox_arg_t *arg) {
    if (!target) {
        target = mbox_list[key];
    }
    if (!target->mailbox_info.initialized) {
        return -1;
    }
    if (arg->sleep_operation == 1 && k_mbox_try_recv(key, arg) < 0) {
        return -2;
    }
    int blocked_time = 0;
    while (arg->msg_length > target->used_units) {
        blocked_time++;
        k_cond_wait(target->empty_cond_id - 1);
    }
    int left_space = MBOX_MSG_MAX_LEN - (target->read_head + arg->msg_length);
    if (left_space < 0) {
        k_memcpy((uint8_t *)arg->msg, (const uint8_t *)target->buff + target->read_head, MBOX_MSG_MAX_LEN - target->read_head);
        k_memcpy((uint8_t *)arg->msg + MBOX_MSG_MAX_LEN - target->read_head, (const uint8_t *)target->buff, -left_space);
        target->read_head = -left_space;
    } else {
        k_memcpy((uint8_t *)arg->msg, (const uint8_t *)target->buff + target->read_head, arg->msg_length);
        target->read_head += arg->msg_length;
    }
    target->used_units -= arg->msg_length;
    k_cond_broadcast(target->full_cond_id - 1);
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

void k_pcb_mbox_init(pcb_mbox_t *target, int owner_id) {
    k_memset((void *)&(target), 0, sizeof(pcb_mbox_t));
    target->pcb_i = owner_id;
}

int sys_mailread(void *buf, int len) {
    pcb_mbox_t *target = &(*current_running)->mbox;
    if (len == 0) {
        if (target->used_units) {
            return 0;
        } else {
            return -1;
        }
    } else if (len > PCB_MBOX_MSG_MAX_LEN) {
        len = PCB_MBOX_MSG_MAX_LEN;
    }
    int str_len = k_strlen((const char *)target->buff[target->read_head]);
    k_memcpy((uint8_t *)buf, (uint8_t *)target->buff[target->read_head], k_min(len, str_len));
    target->read_head = (target->read_head + 1) % PCB_MBOX_MAX_MSG_NUM;
    target->used_units--;
    return len;
}

int sys_mailwrite(int pid, void *buf, int len) {
    pcb_mbox_t *target = &(*current_running)->mbox;
    if (len == 0) {
        if (target->used_units == PCB_MBOX_MAX_MSG_NUM) {
            return -1;
        } else {
            return 0;
        }
    } else if (len > PCB_MBOX_MSG_MAX_LEN) {
        len = PCB_MBOX_MSG_MAX_LEN;
    }
    k_memcpy((uint8_t *)target->buff[target->write_head], (uint8_t *)buf, len);
    target->write_head = (target->write_head + 1) % PCB_MBOX_MAX_MSG_NUM;
    target->used_units++;
    return len;
}