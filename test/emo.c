#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <os.h>

#define MAX_RECV_CNT 130
char recv_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length[MAX_RECV_CNT];

int matoi(char* c)
{
    int num=0;
    for(int i=0;c[i]>='0'&&c[i]<='9';i++)
        num=num*10+(c[i]-'0');
    return num;
}

int main(int argc, char *argv[])
{
    int mode = 0;
    int size = 121;
    if(argc > 1) {
        if (strcmp(argv[1], "1") == 0) {
                mode = 1;
        }
        if(strcmp(argv[1], "2")==0)
            mode = 2;
    }

    sys_net_irq_mode(mode);

    sys_move_cursor(1, 1);
    printf("[RECV TASK] start recv ...");

    int ret = sys_net_recv(recv_buffer, size * sizeof(EthernetFrame), size, recv_length);
    
    char *curr = recv_buffer;
    curr = curr + recv_length[0] + 0x36;
    char lenc[5];
    lenc[0]=recv_buffer[0x36];
    lenc[1]=recv_buffer[0x37];
    lenc[2]=recv_buffer[0x38];
    lenc[3]='\0';
    int len=matoi(lenc);

    printf("done\n");
    int fd=sys_fopen("all2");

    printf("[RECV TASK] start write ...");
    for(int i=1; i<len; i++)
    {
        sys_fwrite(fd, curr, recv_length[i]-0x36);
        curr+=recv_length[i];
    }
    /*
    sys_move_cursor(1, 3);
    for (int i = 0; i < 1; ++i) {
        for (int j = 0; j < (recv_length[i] + 15) / 16; ++j) {
            for (int k = 0; k < 16 && (j * 16 + k < recv_length[i]); ++k) {
                printf("%02x ", (uint32_t)(*(uint8_t*)curr));
                ++curr;
            }
            printf("\n");
        }
    }*/


    printf("done\n");
    sys_close(fd);
    return 0;
}