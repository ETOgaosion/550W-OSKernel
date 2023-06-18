#pragma once

#include <lib/stdarg.h>

/* kernel printf */
int k_print(const char *fmt, ...);
int vk_print(const char *fmt, va_list va);
int prints(const char *fmt, ...);
int vprints(const char *fmt, va_list va);
