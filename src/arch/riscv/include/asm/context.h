#pragma once

#include <common/types.h>

#define NORMAL_REGS_NUM 31

/* used to save register infomation */
typedef struct regs_context {
    /* Saved main processor registers.*/
    reg_t regs[NORMAL_REGS_NUM];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context {
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;
