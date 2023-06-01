#pragma once

#define SAVE(x, y) sd x, OFFSET_REG_##y##(sp)
#define LOAD(x, y) ld x, OFFSET_REG_##y##(sp)

/* return address */
#define reg_ra 0

/* pointers */
#define reg_sp 1 // stack
#define reg_gp 2 // global
#define reg_tp 3 // thread

/* temporary */
#define reg_t0 4
#define reg_t1 5
#define reg_t2 6

/* saved register */
#define reg_s0 7
#define reg_s1 8

/* args */
#define reg_a0 9
#define reg_a1 10
#define reg_a2 11
#define reg_a3 12
#define reg_a4 13
#define reg_a5 14
#define reg_a6 15
#define reg_a7 16

/* saved register */
#define reg_s2 17
#define reg_s3 18
#define reg_s4 19
#define reg_s5 20
#define reg_s6 21
#define reg_s7 22
#define reg_s8 23
#define reg_s9 24
#define reg_s10 25
#define reg_s11 26

/* temporary register */
#define reg_t3 27
#define reg_t4 28
#define reg_t5 29
#define reg_t6 30

/* privileged register */
#define reg_sstatus 31
#define reg_sepc 32
#define reg_sbadaddr 33
#define reg_scause 34

/* offsets */

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

#define OFFSET_REG_SIZE 248

/* privileged register */
#define OFFSET_REG_SSTATUS 248
#define OFFSET_REG_SEPC 256
#define OFFSET_REG_SBADADDR 264
#define OFFSET_REG_SCAUSE 272

/* Size of stack frame, word/double word alignment */
#define OFFSET_ALL_REG_SIZE 280

#define PCB_KERNEL_SP 0
#define PCB_USER_SP 8

/* registers in switch_to */
#define switch_reg_ra 0
#define switch_reg_sp 1
#define switch_reg_s0 2
#define switch_reg_s1 3
#define switch_reg_s2 4
#define switch_reg_s3 5
#define switch_reg_s4 6
#define switch_reg_s5 7
#define switch_reg_s6 8
#define switch_reg_s7 9
#define switch_reg_s8 10
#define switch_reg_s9 11
#define switch_reg_s10 12
#define switch_reg_s11 13

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
