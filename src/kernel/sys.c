#include <lib/string.h>
#include <os/sys.h>

uname_t uname_550w = {.sysname = "MOSS", .nodename = "oscomp", .release = "1.0.0", .version = "#1", .machine = "550W", .domainname = "bluespace.moss.com"};

long sys_uname(uname_t *dest) {
    memcpy((uint8_t *)dest, (uint8_t *)&uname_550w, sizeof(uname_550w));
    return 0;
}