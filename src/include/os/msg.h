#pragma once

#include <common/types.h>
#include <os/ipc.h>

typedef struct msqid_ds {
    ipc_perm_t msg_perm;
    __kernel_old_time_t msg_stime; /* last msgsnd time */
    __kernel_old_time_t msg_rtime; /* last msgrcv time */
    __kernel_old_time_t msg_ctime; /* last change time */
    unsigned short msg_cbytes;     /* current number of bytes on queue */
    unsigned short msg_qnum;       /* number of messages in queue */
    unsigned short msg_qbytes;     /* max number of bytes on queue */
    __kernel_ipc_pid_t msg_lspid;  /* pid of last msgsnd */
    __kernel_ipc_pid_t msg_lrpid;  /* last receive pid */
} msqid_ds_t;

long sys_msgget(key_t key, int msgflg);
long sys_msgctl(int msqid, int cmd, msqid_ds_t *buf);