#pragma once

#include <common/types.h>

typedef struct ipc_perm {
    __kernel_key_t key;
    __kernel_uid_t uid;
    __kernel_gid_t gid;
    __kernel_uid_t cuid;
    __kernel_gid_t cgid;
    __kernel_mode_t mode;
    unsigned short seq;
} ipc_perm_t;