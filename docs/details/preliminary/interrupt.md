# 中断处理

根据中断触发时CPU的状态，我们将中断分为用户中断（user_exception）和内核中断（kernel_exception）两种。

两种中断的处理流程近似，都包括保存上下文->执行中断处理->恢复上下文三个部分。这里给出了两种中断的主体函数。每种都包括进入中断和中断返回两部分。每次中断需要保存/恢复的上下文包括寄存器、栈帧等内容，用于中断结束后恢复应用/进程的执行，内核/用户态下关注的寄存器略有不同。中断处理函数（exception_handler）则根据中断来源调用对应的函数进行处理。

```assembly
// user exception
ENTRY(user_ret_from_exception)
  call k_unlock_kernel
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


// kernel exception
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
```