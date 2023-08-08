#include <lib/string.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/sys.h>
#include <os/time.h>

uname_t uname_550w = {.sysname = "MOSS", .nodename = "oscomp", .release = "6.0.0", .version = "#1", .machine = "550W", .domainname = "bluespace.moss.com"};

char hostname[100] = "bluespace";

sysinfo_t moss_info = {0};

long sys_uname(uname_t *dest) {
    k_memcpy(dest, &uname_550w, sizeof(uname_t));
    return 0;
}

long sys_gethostname(char *name, int len) {
    k_memcpy(name, hostname, len);
    return 0;
}

long sys_sethostname(char *name, int len) {
    k_memcpy(hostname, name, len);
    return 0;
}

void k_sysinfo_init() {
    moss_info.totalram = MEM_SIZE * PAGE_SIZE;
    moss_info.freeram = MEM_SIZE * PAGE_SIZE;
    moss_info.bufferram = 0; /* [NOT CLEAR]: how to calculate? */
    moss_info.mem_unit = 8;  /* [NOT CLEAR]: what's this? */
}

// [TODO]: mantain sysinfo
long sys_sysinfo(sysinfo_t *info) {
    nanotime_val_t now;
    k_time_get_nanotime(&now);
    k_time_minus_nanotime(&now, (nanotime_val_t *)&boot_time, NULL);
    moss_info.uptime = now.sec;
    moss_info.procs = k_pcb_count();
    k_memcpy(info, &moss_info, sizeof(sysinfo_t));
    return 0;
}

void k_meminfo(char *info) {
    char str[50];
    ltoa(moss_info.totalram, str, 10);
    strcat(info, "totalram: ");
    strcat(info, str);
    ltoa(moss_info.freeram, str, 10);
    strcat(info, "freeram: ");
    strcat(info, str);
    ltoa(moss_info.sharedram, str, 10);
    strcat(info, "sharedram: ");
    strcat(info, str);
    ltoa(moss_info.bufferram, str, 10);
    strcat(info, "bufferram: ");
    strcat(info, str);
    ltoa(moss_info.totalswap, str, 10);
    strcat(info, "totalswap: ");
    strcat(info, str);
    ltoa(moss_info.freeswap, str, 10);
    strcat(info, "freeswap: ");
    strcat(info, str);
    ltoa(moss_info.totalhigh, str, 10);
    strcat(info, "totalhigh: ");
    strcat(info, str);
    ltoa(moss_info.freehigh, str, 10);
    strcat(info, "freehigh: ");
    strcat(info, str);
}