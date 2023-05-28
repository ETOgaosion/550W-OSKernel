#pragma once

#define SAVE(x, y) sd x, OFFSET_REG_##y##(sp)
#define LOAD(x, y) ld x, OFFSET_REG_##y##(sp)

/* return address */
#define OFFSET_REG_RA 0

/* pointers */
#define OFFSET_REG_SP 8  // stack
#define OFFSET_REG_GP 16 // global
#define OFFSET_REG_TP 24 // thread

/* temporary */
#define OFFSET_REG_T0 32
#define OFFSET_REG_T1 40
#define OFFSET_REG_T2 48

/* saved register */
#define OFFSET_REG_S0 56
#define OFFSET_REG_S1 64

/* args */
#define OFFSET_REG_A0 72
#define OFFSET_REG_A1 80
#define OFFSET_REG_A2 88
#define OFFSET_REG_A3 96
#define OFFSET_REG_A4 104
#define OFFSET_REG_A5 112
#define OFFSET_REG_A6 120
#define OFFSET_REG_A7 128

/* saved register */
#define OFFSET_REG_S2 136
#define OFFSET_REG_S3 144
#define OFFSET_REG_S4 152
#define OFFSET_REG_S5 160
#define OFFSET_REG_S6 168
#define OFFSET_REG_S7 176
#define OFFSET_REG_S8 184
#define OFFSET_REG_S9 192
#define OFFSET_REG_S10 200
#define OFFSET_REG_S11 208

/* temporary register */
#define OFFSET_REG_T3 216
#define OFFSET_REG_T4 224
#define OFFSET_REG_T5 232
#define OFFSET_REG_T6 240

/* privileged register */
#define OFFSET_REG_SSTATUS 248
#define OFFSET_REG_SEPC 256
#define OFFSET_REG_SBADADDR 264
#define OFFSET_REG_SCAUSE 272

/* Size of stack frame, word/double word alignment */
#define OFFSET_SIZE 280

#define PCB_KERNEL_SP 0
#define PCB_USER_SP 8

/* offset in switch_to */
#define SS(x, y) sd x, SWITCH_TO_##y##(t0)
#define LS(x, y) ld x, SWITCH_TO_##y##(t0)
#define SWITCH_TO_RA 0
#define SWITCH_TO_SP 8
#define SWITCH_TO_S0 16
#define SWITCH_TO_S1 24
#define SWITCH_TO_S2 32
#define SWITCH_TO_S3 40
#define SWITCH_TO_S4 48
#define SWITCH_TO_S5 56
#define SWITCH_TO_S6 64
#define SWITCH_TO_S7 72
#define SWITCH_TO_S8 80
#define SWITCH_TO_S9 88
#define SWITCH_TO_S10 96
#define SWITCH_TO_S11 104

#define SWITCH_TO_SIZE 112
