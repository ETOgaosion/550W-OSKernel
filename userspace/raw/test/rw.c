#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // sys_move_cursor(1, 3);
    // printf("rw: here\n");
    srand(clock());
    long mem2 = 0;
    uintptr_t mem1 = 0;
    int curs = 0;
    int i;
    sys_move_cursor(1, 2);
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; ++i) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    for (i = 1; i < argc; i++) {
        mem1 = atol(argv[i]);
        // sys_move_cursor(2, curs+i);
        mem2 = rand();
        *(long *)mem1 = mem2;
        printf("0x%lx, %ld\n", mem1, mem2);
        if (*(long *)mem1 != mem2) {
            printf("Error!\n");
        }
    }
    // Only input address.
    // Achieving input r/w command is recommended but not required.
    printf("Success!\n");
    // while(1);
    return 0;
}
