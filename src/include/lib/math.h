#pragma once

/* K_ROUNDing; only works for n = power of two */
#define K_ROUND(a, n) (((((uint64_t)(a)) + (n)-1)) & ~((n)-1))
#define K_ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

#define DOWN_ALIGN(x, y) (((u64)(x)) & (~((u64)((y)-1))))
#define UP_ALIGN(x, y) ((DOWN_ALIGN((x)-1, (y))) + (y))

#define k_wrap(x, len) ((x) & ~(len))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int k_min(int a, int b);

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) > (y)) ? (y) : (x))