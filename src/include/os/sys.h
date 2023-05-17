#pragma once

typedef struct uname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
} uname_t;

extern uname_t uname_550w;

long sys_uname(uname_t *);