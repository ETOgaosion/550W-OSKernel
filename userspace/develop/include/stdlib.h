#pragma once

void panic(char *);

#define WEXITSTATUS(s) (((s)&0xff00) >> 8)

/* clang-format off */
#ifndef assert
#define assert(f) \
    if (!(f))     \
	panic("\n --- Assert Fatal ! ---\n")
#endif
/* clang-format on */