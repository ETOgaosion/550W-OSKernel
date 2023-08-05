#include <lib/string.h>
#include <os/users.h>

user_t users[USERS_NUM_MAX];

void k_users_init() {
    users[0].uid = 0;
    k_strcpy(users[0].name, "root");
    k_strcpy(users[0].user_name, "root");
    k_strcpy(users[0].pass_word, "root");
}

long sys_acct(const char *name) {
    return 0;
}

long sys_getgroups(int gidsetsize, gid_t *grouplist) {
    return 0;
}

long sys_setgroups(int gidsetsize, gid_t *grouplist) {
    return 0;
}