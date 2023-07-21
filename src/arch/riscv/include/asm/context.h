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
    reg_t stval;
    reg_t scause;
    reg_t sscratch;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context {
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;

// for the user signal
typedef unsigned long riscv_mc_gp_state[32];

typedef struct riscv_mc_f_ext_state {
    unsigned int __f[32];
    unsigned int __fcsr;
} riscv_mc_f_ext_state_t;

typedef struct riscv_mc_d_ext_state {
    unsigned long long __f[32];
    unsigned int __fcsr;
} riscv_mc_d_ext_state_t;

typedef struct riscv_mc_q_ext_state {
    unsigned long long __f[64] __attribute__((aligned(16)));
    unsigned int __fcsr;
    unsigned int __reserved[3];
} riscv_mc_q_ext_state_t;

typedef union riscv_mc_fp_state {
    riscv_mc_f_ext_state_t __f;
    riscv_mc_d_ext_state_t __d;
    riscv_mc_q_ext_state_t __q;
} riscv_mc_fp_state_t;

typedef struct asm_mcontext {
    riscv_mc_gp_state __gregs;
    riscv_mc_fp_state_t __fpregs;
} asm_mcontext_t;