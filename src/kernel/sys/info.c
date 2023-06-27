#include <lib/string.h>
#include <os/sys.h>

uname_t uname_550w = {.sysname = "MOSS", .nodename = "oscomp", .release = "1.0.0", .version = "#1", .machine = "550W", .domainname = "bluespace.moss.com"};

char hostname[100] = "bluespace";

sysinfo_t sys_info = {0};

long sys_uname(uname_t *dest) {
    k_memcpy((uint8_t *)dest, (uint8_t *)&uname_550w, sizeof(uname_t));
    return 0;
}

long sys_gethostname(char *name, int len) {
    k_memcpy((uint8_t *)name, (uint8_t *)hostname, len);
    return 0;
}

long sys_sethostname(char *name, int len) {
    k_memcpy((uint8_t *)hostname, (uint8_t *)name, len);
    return 0;
}

// [TODO]: mantain sysinfo
long sys_sysinfo(sysinfo_t *info) {
    k_memcpy((uint8_t *)info, (uint8_t *)&sys_info, sizeof(sysinfo_t));
    return 0;
}