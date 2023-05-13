#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

int matoi(char* c)
{
    int num=0;
    for(int i=0;c[i]>='0'&&c[i]<='9';i++)
        num=num*10+(c[i]-'0');
    return num;
}

int main(int argc, char *argv[])
{
    if(argc > 1)
    {
        sys_move_cursor(1, 10);
        //printf("argc:%d, argv:%s\n",argc, argv[1]);
        int fd=sys_fopen("all.txt", O_RDWR);
        int pos=matoi(argv[1]);
        
        int size=70;

        sys_lseek(fd, pos*size, 0);
        
        //printf("here\n");

        char curr[20];
        int len=size;
        printf("pocket : [%d]\n", pos);
        for (int j = 0; j < (len + 15) / 16; ++j) {
            for (int k = 0; k < 16 && (j * 16 + k < len); ++k) {
                sys_fread(fd, curr, 1);
                printf("%02x ", (uint32_t)(*(uint8_t*)curr));
            }
            printf("\n");
        }
        sys_close(fd);
    }
    else
    {
        printf("usage : exec check seek_pos");
        return 0;
    }
    return 0;
}