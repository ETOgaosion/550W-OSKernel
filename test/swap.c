#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>

int main()
{
	srand(clock());
    uint64_t data;
    int i;
    int num=0;
    int test_sum=100;
    uint64_t mem_addr = 0x100001;
    int a[1005];
	for(i=0; i<test_sum; i++)
    {
        data = rand();
        a[i] = data;
        *(long*)mem_addr = data;
        sys_move_cursor(1, 2);
        printf("[%d] write to 0x%x ...\n", i, mem_addr);
        mem_addr += 0x1000;
        sys_sleep(1);
    }
    mem_addr = 0x100001;
    for(i=0; i<test_sum; i++)
    {
        if(*(long*)mem_addr == a[i])
        {
            num++;
            sys_move_cursor(1, 3);
            printf("[%d] Success for 0x%x!\n", i, mem_addr);
        }
        else
        {
            sys_move_cursor(1, 3);
            printf("[%d] Failed for %d: %d, should be %d\n", i, mem_addr, *(long*)mem_addr, a[i]);
            //break;
        }
        mem_addr+=0x1000;
        sys_sleep(1);
    } 
	if(num==test_sum)
        printf("TEST PASS!\n");
    else
        printf("TEST FAILED!\n");
	//Only input address.
	//Achieving input r/w command is recommended but not required.
	// while(1);
	return 0;
}
