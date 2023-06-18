#include <asm/common.h>
#include <asm/io.h>
#include <asm/sbi.h>
#include <os/smp.h>

void k_port_write_ch(char ch) {
    sbi_console_putchar((int)ch);
}

void k_port_write(char *str) {
    sbi_console_putstr(str);
}

long k_port_read() {
    return sbi_console_getchar();
}
