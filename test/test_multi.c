#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <mthread.h>

#include <os.h>

#define MAX_ITERATION 100
#define INTEGER_TEST_CHUNK 900

int main()
{
    uint64_t pid = sys_getpid();
    uint64_t ans = 0;
    int num=0;
    while(1)
    {
        for (int j = 0; j < INTEGER_TEST_CHUNK; ++j) 
            ans += rand();
	
        sys_move_cursor(1, 9);
        printf("[%ld] integer test (%d)\n\r", pid, num++);
    }
}