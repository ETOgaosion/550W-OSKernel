#include <stdio.h>
#include <sys/syscall.h>
#include <test2.h>

void fork_task1(void) {
    int i;
    int print_location = 7;
    int id = -1;
    int putin = -1;
    // new proc by forked is new father proc
    for (i = 0;; i++) {
        if (id == -1 || id == 0) {
            putin = sys_read();
            if (putin != -1) {
                id = sys_fork(putin - 48);
                if (id == 0) {
                    print_location++;
                }
            }
        }
        if (id < 20 && id > 0 || id == -1) {
            sys_move_cursor(1, print_location);
            printf("> [TASK] This is father process.(%d)", i);
        } else if (id == 0) {
            sys_move_cursor(1, print_location);
            printf("> [TASK] This is child process. (%d)", i);
        } else if (id == 20) {
            sys_move_cursor(1, print_location);
            printf("> [ERROR] fork out of limited times! (%d)", i);
        }
        // sys_yield();
    }
}