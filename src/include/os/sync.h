#pragma once

#include <lib/list.h>

#define SYNC_NUM 32
#define MBOX_NAME_LEN 64
#define MBOX_MSG_MAX_LEN 128
#define MBOX_MAX_USER 10

#define PCB_MBOX_MAX_MSG_NUM 16
#define PCB_MBOX_MSG_MAX_LEN 256

/* semop flags */
#define SEM_UNDO 0x1000 /* undo the operation on exit */

/* semctl Command Definitions. */
#define GETPID 11  /* get sempid */
#define GETVAL 12  /* get semval */
#define GETALL 13  /* get all semval's */
#define GETNCNT 14 /* get semncnt */
#define GETZCNT 15 /* get semzcnt */
#define SETVAL 16  /* set semval */
#define SETALL 17  /* set all semval's */

/* ipcs ctl cmds */
#define SEM_STAT 18
#define SEM_INFO 19
#define SEM_STAT_ANY 20

#define SEMMNI 32000             /* <= IPCMNI  max # of semaphore identifiers */
#define SEMMSL 32000             /* <= INT_MAX max num of semaphores per id */
#define SEMMNS (SEMMNI * SEMMSL) /* <= INT_MAX max # of semaphores in system */
#define SEMOPM 500               /* <= 1 000 max num of ops per semop call */
#define SEMVMX 32767             /* <= 32767 semaphore maximum value */
#define SEMAEM SEMVMX            /* adjust on exit max value */

/* unused */
#define SEMUME SEMOPM /* max num of undo entries per process */
#define SEMMNU SEMMNS /* num of undo structures system wide */
#define SEMMAP SEMMNS /* # of entries in semaphore map */
#define SEMUSZ 20     /* sizeof struct sem_undo */

typedef struct basic_info {
    int id;
    key_t key;
    bool public;
    int initialized;
} basic_info_t;

typedef struct semaphore {
    basic_info_t sem_info;
    int sem;
    list_head wait_queue;
} semaphore_t;

typedef struct sembuf {
    unsigned short sem_num; /* semaphore index in array */
    short sem_op;           /* semaphore operation */
    short sem_flg;          /* operation flags */
} sembuf_t;

typedef struct seminfo {
    int semmap;
    int semmni;
    int semmns;
    int semmnu;
    int semmsl;
    int semopm;
    int semume;
    int semusz;
    int semvmx;
    int semaem;
} seminfo_t;

typedef struct cond {
    basic_info_t cond_info;
    int num_wait;
    list_head wait_queue;
} cond_t;

typedef struct barrier {
    basic_info_t barrier_info;
    int count;
    int total;
    // int mutex_id;
    int cond_id;
} barrier_t;

typedef struct mbox {
    basic_info_t mailbox_info;
    int id[2];
    char buff[MBOX_MSG_MAX_LEN];
    int read_head, write_tail;
    int used_units;
    // int mutex_id;
    int full_cond_id;
    int empty_cond_id;
} mbox_t;

typedef struct mbox_arg {
    void *msg;
    int msg_length;
    int sleep_operation;
} mbox_arg_t;

typedef struct pcb_mbox {
    int pcb_i;
    char buff[PCB_MBOX_MAX_MSG_NUM][PCB_MBOX_MSG_MAX_LEN];
    int read_head, write_head;
    int used_units;
} pcb_mbox_t;

int k_commop(void *key_id, void *arg, int op);

int k_semaphore_init(int *key, int sem);
int k_semaphore_p(int key, int value, int flag);
int k_semaphore_v(int key, int value, int flag);
int k_semaphore_destroy(int *key);

int k_cond_init(int *key);
// int k_cond_wait(int key, int lock_id);
int k_cond_wait(int key);
int k_cond_signal(int key);
int k_cond_broadcast(int key);
int k_cond_destroy(int *key);

int k_barrier_init(int *key, int total);
int k_barrier_wait(int key);
int k_barrier_destroy(int *key);

int k_mbox_open(int id_1, int id_2);
int k_mbox_close();
int k_mbox_send(int key, mbox_t *target, mbox_arg_t *arg);
int k_mbox_recv(int key, mbox_t *target, mbox_arg_t *arg);
int k_mbox_try_send(int key, mbox_arg_t *arg);
int k_mbox_try_recv(int key, mbox_arg_t *arg);

void k_pcb_mbox_init(pcb_mbox_t *target, int owner_id);

long sys_mailread(void *buf, int len);
long sys_mailwrite(int pid, void *buf, int len);

long sys_semget(key_t key, int nsems, int semflg);
long sys_semctl(int semid, int semnum, int cmd, unsigned long arg);
long sys_semop(int semid, sembuf_t *sops, unsigned nsops);