
#include <asm/asm.h>
#include <asm/csr.h>

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* setup C environment */
  //lw sp, pid0_stack
  csrw CSR_SSCRATCH, x0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  csrr t0, CSR_MHARTID
  bnez t0, slave

  //these instructions will modify memory
  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sd zero, (a3)
  add a3, a3, RISCV_SZPTR
  blt a3, a4, clear_bss
clear_bss_done:

  // core 0
master:
  li sp, BOOT_STACK_MASTER
  csrr a0, CSR_MHARTID 
  call prepare_vm
slave:
  /* Mark all interrupt */
  li t0, SR_FS
  csrc sstatus, t0
  /* Set stvec to kernel entry */
  csrw CSR_SSCRATCH, t0
  la t0, prepare_vm
  csrw CSR_STVEC, t0
  /* Enable supervisor software interrupt (ipi) */
  li t0, SIE_SSIE
  csrw CSR_SIE, t0
  li t0, SR_SIE
  csrw CSR_SSTATUS, t0
  li sp, BOOT_STACK_SLAVE

loop:
  wfi
  j loop

END(_start)
