#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

static char buff[64];

int main(void) {
    int i, j;
    int fd = sys_fopen("bigfile.txt", O_RDWR);

    // write 'hello world!' * 10
    for (i = 0; i < 10; i++) {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    if (sys_lseek(fd, 0x1000000, 0) == 0) {
        printf("lseek error\n");
    }

    int sum = 0;
    for (i = 0; i < 10; i++) {
        sum += sys_fwrite(fd, "goodbye world!\n", 15);
    }

    // printf("sum :%d\n", sum);
    if (sys_lseek(fd, -sum, 1) == 0) {
        printf("lseek error\n");
    }

    for (i = 0; i < 10; i++) {
        sys_fread(fd, buff, 15);
        for (j = 0; j < 15; j++) {
            printf("%c", buff[j]);
        }
    }

    sys_close(fd);
}