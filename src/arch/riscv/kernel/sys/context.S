#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/regs.h>

.macro SAVE_CONTEXT
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
  //SAVE(sp,SP)
  SAVE(gp,GP)
  SAVE(tp,TP)
  SAVE(t0,T0)
  SAVE(t1,T1)
  SAVE(t2,T2)
  SAVE(s0,S0)
  SAVE(s1,S1)
  SAVE(a0,A0)
  SAVE(a1,A1)
  SAVE(a2,A2)
  SAVE(a3,A3)
  SAVE(a4,A4)
  SAVE(a5,A5)
  SAVE(a6,A6)
  SAVE(a7,A7)
  SAVE(s2,S2)
  SAVE(s3,S3)
  SAVE(s4,S4)
  SAVE(s5,S5)
  SAVE(s6,S6)
  SAVE(s7,S7)
  SAVE(s8,S8)
  SAVE(s9,S9)
  SAVE(s10,S10)
  SAVE(s11,S11)
  SAVE(t3,T3)
  SAVE(t4,T4)
  SAVE(t5,T5)
  SAVE(t6,T6)
  csrr a0, CSR_SSTATUS
  SAVE(a0,SSTATUS)
  csrr a0, CSR_SEPC
  SAVE(a0,SEPC)
  csrr a0, CSR_SCAUSE
  SAVE(a0,SCAUSE)
  ld a0, PCB_USER_SP(tp)
  SAVE(a0,SP)
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

.macro RESTORE_CONTEXT
  ld sp, PCB_KERNEL_SP(tp)
  addi sp, sp, SWITCH_TO_SIZE
  LOAD(ra,RA)
  LOAD(gp,GP)
  LOAD(tp,TP)
  LOAD(t0,T0)
  LOAD(t1,T1)
  LOAD(t2,T2)
  //LOAD(s0,S0)
  LOAD(s1,S1)
  LOAD(a0,A0)
  LOAD(a1,A1)
  LOAD(a2,A2)
  LOAD(a3,A3)
  LOAD(a4,A4)
  LOAD(a5,A5)
  LOAD(a6,A6)
  LOAD(a7,A7)
  LOAD(s2,S2)
  LOAD(s3,S3)
  LOAD(s4,S4)
  LOAD(s5,S5)
  LOAD(s6,S6)
  LOAD(s7,S7)
  LOAD(s8,S8)
  LOAD(s9,S9)
  LOAD(s10,S10)
  LOAD(s11,S11)
  LOAD(t3,T3)
  LOAD(t4,T4)
  LOAD(t5,T5)
  LOAD(t6,T6)
  LOAD(s0,SSTATUS)
  csrw CSR_SSTATUS, s0
  LOAD(s0,SEPC)
  csrw CSR_SEPC, s0
  LOAD(s0,S0)
  LOAD(sp,SP)
  fence
  /*ld sstatus, 256(sp)
  ld sepc, 264(sp)
  ld sbadaddr, 272(sp)
  ld scause, 280(sp)*/
  /* TODO: Restore all registers and sepc,sstatus */
.endm

ENTRY(enable_preempt)
  jr ra
  ld t1, current_running0
  ld t0, PCB_PREEMPT_COUNT(t1)
  beq t0, zero, do_enable
  addi t0, t0, -1
  sd t0, PCB_PREEMPT_COUNT(t1)
  beq t0, zero, do_enable
  jr ra
do_enable:
  not t0, x0
  csrs CSR_SIE, t0
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
  jr ra
  csrw CSR_SIE, zero
  ld t1, current_running0
  ld t0, PCB_PREEMPT_COUNT(t1)
  addi t0, t0, 1
  sd t0, PCB_PREEMPT_COUNT(t1)
  jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// the address of previous pcb in a0
// the address of next pcb in a1
ENTRY(switch_to)
  // save all callee save registers on kernel stack
  //addi sp, sp, -(SWITCH_TO_SIZE)
  /* TODO: store all callee save registers,
   * see the definition of `struct switchto_context` in pcb.h*/
  
  ld t0, PCB_KERNEL_SP(a0)
  SS(ra,RA)
  SS(sp,SP)
  SS(s0,S0)
  SS(s1,S1)
  SS(s2,S2)
  SS(s3,S3)
  SS(s4,S4)
  SS(s5,S5)
  SS(s6,S6)
  SS(s7,S7)
  SS(s8,S8)
  SS(s9,S9)
  SS(s10,S10)
  SS(s11,S11)
  // restore next
  /* TODO: restore all callee save registers,
   * see the definition of `struct switchto_context` in pcb.h*/
  addi tp, a1, 0
  ld t0, PCB_KERNEL_SP(a1)
  //must sd and ld ra, or return from handlerint,
  //as change sp and many things, can't return 
  //from 
  LS(ra,RA)
  LS(sp,SP)
  LS(s0,S0)
  LS(s1,S1)
  LS(s2,S2)
  LS(s3,S3)
  LS(s4,S4)
  LS(s5,S5)
  LS(s6,S6)
  LS(s7,S7)
  LS(s8,S8)
  LS(s9,S9)
  LS(s10,S10)
  LS(s11,S11)

  jr ra
  //csrw CSR_SEPC, ra
  //sret
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  /* TODO: */
  RESTORE_CONTEXT
  sret
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)
  SAVE_CONTEXT

  csrw CSR_SSCRATCH, x0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  mv a0, sp
  csrr a1, CSR_STVAL
  csrr a2, CSR_SCAUSE
  csrr a3, CSR_MHARTID
  ld sp, PCB_KERNEL_SP(tp)
  la t0, interrupt_helper
  jr t0
  la ra, ret_from_exception
  jr ra
ENDPROC(exception_handler_entry)
