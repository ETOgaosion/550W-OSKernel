#include <asm/common.h>
#include <asm/sbi.h>
#include <os/string.h>

void port_write_ch(char ch)
{
    sbi_console_putchar((int)ch);
}

void port_write(char *str)
{
    sbi_console_putstr(str);
}

int port_read()
{
    return sbi_console_getchar();
}