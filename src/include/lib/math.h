#pragma once

/* Rounding; only works for n = power of two */
#define ROUND(a, n) (((((uint64_t)(a)) + (n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

#define wrap(x, len) ((x) & ~(len))

int k_min(int a, int b);