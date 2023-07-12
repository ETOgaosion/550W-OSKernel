#pragma once

#include <common/types.h>

#define USER_NAME_LEN 64
#define USERS_NUM_MAX 16

typedef struct user {
    uid_t uid;
    char name[USER_NAME_LEN];
    char user_name[USER_NAME_LEN];
    char pass_word[USER_NAME_LEN];
} user_t;

extern user_t users[USERS_NUM_MAX];

void init_users();
long sys_acct(const char *name);
long sys_getgroups(int gidsetsize, gid_t *grouplist);
long sys_setgroups(int gidsetsize, gid_t *grouplist);