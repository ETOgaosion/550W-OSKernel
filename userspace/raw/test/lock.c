#include <stdio.h>
#include <sys/syscall.h>

// LOCK2_BINSEM_KEY is the key of this task. You can define it as you wish.
// We use 42 here because it is "Answer to the Ultimate Question of Life,
// the Universe, and Everything" :)
#define LOCK2_BINSEM_KEY 42

static char blank[] = {"                                             "};

int main(int argc, char *argv[]) {
    int print_location = 1;
    int binsem_id = binsemget(LOCK2_BINSEM_KEY);
    if (argc > 1) {
        // maybe we should implement an `atoi` in tinylibc?
        print_location = (int)atol(argv[1]);
    }
    while (1) {
        int i;

        sys_move_cursor(1, print_location);
        printf("%s", blank);

        sys_move_cursor(1, print_location);
        printf("> [TASK] Applying for a lock.\n");

        binsemop(binsem_id, BINSEM_OP_LOCK);

        for (i = 0; i < 2000; i++) {
            sys_move_cursor(1, print_location);
            printf("> [TASK] Has acquired lock and running.(%d)\n", i);
        }

        sys_move_cursor(1, print_location);
        printf("%s", blank);

        sys_move_cursor(1, print_location);
        printf("> [TASK] Has acquired lock and exited.\n");

        binsemop(binsem_id, BINSEM_OP_UNLOCK);
    }

    return 0;
}
