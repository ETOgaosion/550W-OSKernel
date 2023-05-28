#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void panic(char *m) {
    puts(m);
    exit(-100);
}
