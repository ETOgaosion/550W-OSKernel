#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/regs.h>

.macro SAVE_REGS
  SAVE(ra, RA)
  SAVE(gp, GP)
  SAVE(tp, TP)
  SAVE(t0, T0)
  SAVE(t1, T1)
  SAVE(t2, T2)
  SAVE(s0, S0)
  SAVE(s1, S1)
  SAVE(a0, A0)
  SAVE(a1, A1)
  SAVE(a2, A2)
  SAVE(a3, A3)
  SAVE(a4, A4)
  SAVE(a5, A5)
  SAVE(a6, A6)
  SAVE(a7, A7)
  SAVE(s2, S2)
  SAVE(s3, S3)
  SAVE(s4, S4)
  SAVE(s5, S5)
  SAVE(s6, S6)
  SAVE(s7, S7)
  SAVE(s8, S8)
  SAVE(s9, S9)
  SAVE(s10, S10)
  SAVE(s11, S11)
  SAVE(t3, T3)
  SAVE(t4, T4)
  SAVE(t5, T5)
  SAVE(t6, T6)
  csrr a0, sstatus
  SAVE(a0, SSTATUS)
  csrr a0, sepc
  SAVE(a0, SEPC)
  csrr a0, scause
  SAVE(a0, SCAUSE)
.endm

.macro LOAD_REGS
  LOAD(ra, RA)
  LOAD(gp, GP)
  LOAD(tp, TP)
  LOAD(t0, T0)
  LOAD(t1, T1)
  LOAD(t2, T2)
  LOAD(s0, S0)
  LOAD(s1, S1)
  LOAD(a0, A0)
  LOAD(a1, A1)
  LOAD(a2, A2)
  LOAD(a3, A3)
  LOAD(a4, A4)
  LOAD(a5, A5)
  LOAD(a6, A6)
  LOAD(a7, A7)
  LOAD(s2, S2)
  LOAD(s3, S3)
  LOAD(s4, S4)
  LOAD(s5, S5)
  LOAD(s6, S6)
  LOAD(s7, S7)
  LOAD(s8, S8)
  LOAD(s9, S9)
  LOAD(s10, S10)
  LOAD(s11, S11)
  LOAD(t3, T3)
  LOAD(t4, T4)
  LOAD(t5, T5)
  LOAD(t6, T6)
  LOAD(s0, SSTATUS)
  csrw sstatus, s0
  LOAD(s0, SEPC)
  csrw sepc, s0
  LOAD(s0, S0)
.endm

.macro USER_SAVE_CONTEXT
//  .local _restore_kernel_tpsp
  .local _save_context
  /*
   * If coming from userspace, preserve the user thread pointer and load
   * the kernel thread pointer.  If we came from the kernel, sscratch
   * will contain 0, and we should continue on the current TP.
   */
//  csrrw tp, CSR_SSCRATCH, tp
//  bnez tp, _save_context

//_restore_kernel_tpsp:
//  csrr tp, CSR_SSCRATCH
//  sd sp, PCB_KERNEL_SP(tp)
_save_context:
  sd sp, PCB_USER_SP(tp)
  ld sp, PCB_KERNEL_SP(tp)
  addi sp, sp, SWITCH_TO_SIZE

  /* TODO: save all general purpose registers here! */
  //SAVE(sp, SP)
  SAVE_REGS
  ld a0, PCB_USER_SP(tp)
  SAVE(a0, SP)
  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  li t0, SR_SUM | SR_FS
  //addi sp, sp, -(SWITCH_TO_SIZE)
  /* TODO: save sstatus, sepc, stval, scause and sscratch on user stack */
.endm

.macro USER_RESTORE_CONTEXT
  .local _restore_context
_restore_context:
  ld sp, PCB_KERNEL_SP(tp)
  addi sp, sp, SWITCH_TO_SIZE
  LOAD_REGS
  LOAD(sp, SP)
  fence
  /*ld sstatus, 256(sp)
  ld sepc, 264(sp)
  ld sbadaddr, 272(sp)
  ld scause, 280(sp)*/
  /* TODO: Restore all registers and sepc, sstatus */
.endm

// the address of previous pcb in a0
// the address of next pcb in a1
ENTRY(switch_to)
  // save all callee save registers on kernel stack
  //addi sp, sp, -(SWITCH_TO_SIZE)
  /* TODO: store all callee save registers,
   * see the definition of `struct switchto_context` in pcb.h*/
  beq a0, a1, switch_to_same_pcb

  ld t0, PCB_KERNEL_SP(a0)
  SS(ra, RA)
  SS(sp, SP)
  SS(s0, S0)
  SS(s1, S1)
  SS(s2, S2)
  SS(s3, S3)
  SS(s4, S4)
  SS(s5, S5)
  SS(s6, S6)
  SS(s7, S7)
  SS(s8, S8)
  SS(s9, S9)
  SS(s10, S10)
  SS(s11, S11)
  // restore next
  /* TODO: restore all callee save registers,
   * see the definition of `struct switchto_context` in pcb.h*/
  addi tp, a1, 0
  ld t0, PCB_KERNEL_SP(a1)
  //must sd and ld ra, or return from handlerint,
  //as change sp and many things, can t return 
  //from 
  LS(ra, RA)
  LS(sp, SP)
  LS(s0, S0)
  LS(s1, S1)
  LS(s2, S2)
  LS(s3, S3)
  LS(s4, S4)
  LS(s5, S5)
  LS(s6, S6)
  LS(s7, S7)
  LS(s8, S8)
  LS(s9, S9)
  LS(s10, S10)
  LS(s11, S11)

switch_to_same_pcb:
  ret
  //csrw CSR_SEPC, ra
  //sret
ENDPROC(switch_to)

ENTRY(user_ret_from_exception)
  /* TODO: */
  call k_smp_unlock_kernel
  la t0, user_exception_handler_entry
  csrw stvec, t0
  la t0, SR_SIE
  csrc sstatus, t0
  la t0, SIE_SEIE | SIE_SSIE | SIE_STIE
  csrw sie, t0
  USER_RESTORE_CONTEXT
  sret
ENDPROC(user_ret_from_exception)

ENTRY(user_exception_handler_entry)
  USER_SAVE_CONTEXT

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  mv a0, sp
  csrr a1, stval
  csrr a2, scause
  csrr a3, sscratch
  ld sp, PCB_KERNEL_SP(tp)
  call user_interrupt_helper
  call user_ret_from_exception
ENDPROC(user_exception_handler_entry)

ENTRY(kernel_ret_from_exception)
  LOAD_REGS
  addi sp, sp, OFFSET_ALL_REG_SIZE
  sret
ENDPROC(kernel_ret_from_exception)

ENTRY(kernel_exception_handler_entry)
  add sp, sp, -OFFSET_ALL_REG_SIZE
  SAVE_REGS

  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  mv a0, sp
  csrr a1, stval
  csrr a2, scause
  csrr a3, sscratch
  call kernel_interrupt_helper
  call kernel_ret_from_exception
ENDPROC(kernel_exception_handler_entry)