#include "stdio.h"

/*
 * for execve
 */

int main(int argc, char *argv[]) {
    printf("  I am %s.\nexecve success.\n", argv[0]);
    TEST_END(__func__);
    return 0;
}
