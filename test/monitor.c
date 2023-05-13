#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <mthread.h>

#include <os.h>

#define MAX_RECV_CNT 64
char recv_buffer1[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length1[MAX_RECV_CNT];

char recv_buffer2[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length2[MAX_RECV_CNT];

void recv1()
{
    int num_packet=0;
    sys_net_port(50001);
    sys_move_cursor(1, 2);
    printf("[RECV PORT : 50001] start recv:                    ");
    while(1)
    {
        sys_net_recv(recv_buffer1, sizeof(EthernetFrame), 1, recv_length1);
        char *curr = recv_buffer1;
        
        sys_move_cursor(1, 2);
        for (int i = 0; i < 1; ++i) {
            printf("packet %d:\n", ++num_packet);
            for (int j = 0; j < (recv_length1[i] + 15) / 16; ++j) {
                for (int k = 0; k < 16 && (j * 16 + k < recv_length1[i]); ++k) {
                    printf("%02x ", (uint32_t)(*(uint8_t*)curr));
                    ++curr;
                }
                printf("\n");
            }
        }
    }

}

void recv2()
{
    int num_packet = 0;
    sys_net_port(58688);
    sys_move_cursor(1, 10);
    printf("[RECV PORT : 58688] start recv:                    ");
    while(1)
    {
        sys_net_recv(recv_buffer2, sizeof(EthernetFrame), 1, recv_length2);
        char *curr = recv_buffer2;
        
        sys_move_cursor(1, 11);
        for (int i = 0; i < 1; ++i) {
            printf("packet %d:\n", ++num_packet);
            for (int j = 0; j < (recv_length2[i] + 15) / 16; ++j) {
                for (int k = 0; k < 16 && (j * 16 + k < recv_length2[i]); ++k) {
                    printf("%02x ", (uint32_t)(*(uint8_t*)curr));
                    ++curr;
                }
                printf("\n");
            }
        }
    }

}

int main(int argc, char *argv[])
{
    sys_net_irq_mode(4);

    mthread_t id1, id2;

    mthread_create(&id1, recv1, 0);
    mthread_create(&id2, recv2, 0);
    mthread_join(id1);
    mthread_join(id2);

    return 0;
}