#include <mthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
mthread_barrier_t barrier;

void clear(int i) {
    printf("                                                                       ");
    sys_move_cursor(1, i);
}

uint64_t a[100];
int main() {
    mthread_barrier_init(&barrier, 2);
    srand(clock());
    uint64_t data;
    int i;
    int num = 0;
    int test_sum = 10;
    int id = -1;
    uint64_t mem_addr = 0x1;
    uint64_t mem_addr2 = 0x1;
    for (i = 0; i < test_sum; i++) {
        data = rand();
        a[i] = data;
        *(long *)mem_addr = data;
        sys_move_cursor(1, 2);
        printf("[%d] write to 0x%x ...\n", i, mem_addr);
        mem_addr += 0x1000;
    }
    mem_addr = 0x1;
    id = sys_fork();

    if (id != 0) {
        srand(clock());
        sys_move_cursor(1, 2);
        clear(2);
        printf("Father read, Child read ... [BEFORE write]\n");
        for (int i = 0; i < test_sum; i++) {
            mem_addr = 0x1 + i * 0x1000;
            sys_move_cursor(1, 3);
            data = *(long *)mem_addr;
            clear(3);
            printf("[%d] [FATHER] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr, sys_getpa(mem_addr));
            sys_sleep(1);
            mthread_barrier_wait(&barrier);
        }
        sys_move_cursor(1, 2);
        clear(2);
        printf("Father read after write, Child only read ...\n");
        for (int i = 0; i < test_sum / 2; i++) {
            mem_addr = 0x1 + i * 0x1000;
            data = rand();
            *(long *)mem_addr = data;
            sys_move_cursor(1, 3);
            clear(3);
            printf("[%d] [FATHER] data %d written to  0x%x, pa is 0x%x\n", i, data, mem_addr, sys_getpa(mem_addr));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);

            data = *(long *)mem_addr;
            sys_move_cursor(1, 3);
            clear(3);
            printf("[%d] [FATHER] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr, sys_getpa(mem_addr));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);
        }
        sys_move_cursor(1, 2);
        clear(2);
        printf("Father only read, Child read after write ...\n");
        for (int i = test_sum / 2; i < test_sum; i++) {
            mem_addr = 0x1 + i * 0x1000;
            data = *(long *)mem_addr;
            sys_move_cursor(1, 3);
            clear(3);
            printf("[%d] [FATHER] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr, sys_getpa(mem_addr));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);

            data = *(long *)mem_addr;
            sys_move_cursor(1, 3);
            clear(3);
            printf("[%d] [FATHER] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr, sys_getpa(mem_addr));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);
        }
        sys_waitpid(id);
    } else {
        srand(0);
        for (int i = 0; i < test_sum; i++) {
            mem_addr2 = 0x1 + i * 0x1000;
            data = *(long *)mem_addr2;
            sys_move_cursor(1, 4);
            clear(4);
            printf("[%d] [CHILD] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr2, sys_getpa(mem_addr2));
            sys_sleep(1);
            mthread_barrier_wait(&barrier);
        }
        for (int i = 0; i < test_sum / 2; i++) {
            mem_addr2 = 0x1 + i * 0x1000;
            data = *(long *)mem_addr2;
            sys_move_cursor(1, 4);
            clear(4);
            printf("[%d] [CHILD] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr2, sys_getpa(mem_addr2));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);

            data = *(long *)mem_addr2;
            sys_move_cursor(1, 4);
            clear(4);
            printf("[%d] [CHILD] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr2, sys_getpa(mem_addr2));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);
        }
        for (int i = test_sum / 2; i < test_sum; i++) {
            mem_addr2 = 0x1 + i * 0x1000;
            data = rand();
            *(long *)mem_addr2 = data;
            sys_move_cursor(1, 4);
            clear(4);
            printf("[%d] [CHILD] data %d written to  0x%x, pa is 0x%x\n", i, data, mem_addr2, sys_getpa(mem_addr2));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);

            data = *(long *)mem_addr2;
            sys_move_cursor(1, 4);
            clear(4);
            printf("[%d] [CHILD] data %d readed from 0x%x, pa is 0x%x\n", i, data, mem_addr2, sys_getpa(mem_addr2));
            sys_sleep(2);
            mthread_barrier_wait(&barrier);
        }
    }
    // Only input address.
    // Achieving input r/w command is recommended but not required.
    //  while(1);
    return 0;
}
