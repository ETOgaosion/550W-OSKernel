
#include <asm/asm.h>
#include <asm/csr.h>

.section ".entry_function","ax"
ENTRY(_start)

  /* Use sscratch to get hart id */
  csrw sscratch, a0
  bnez a0, core2

  /* Mask all interrupts */
  csrw sie, zero
  csrw sip, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sd zero, (a3)
  add a3, a3, RISCV_SZPTR
  blt a3, a4, clear_bss
clear_bss_done:

  /* Set up stack and prepare vm */
  li sp, 0x80501000
  call prepare_vm
  call main

core2:
	csrw sie, zero
  csrw sip, zero
  //la t0, core2_next
  //csrw CSR_STVEC, t0
	//li t0, 0x2
  //csrw CSR_SIE, t0
  //li t0, SR_SIE
  //csrw CSR_SSTATUS, t0

  li t0, SR_FS
  csrc sstatus, t0
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  
  li sp, 0x80502000
  call prepare_vm
  call main
  
  ret

END(_start)
