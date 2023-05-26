#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/sbidef.h>

.macro SBI_CALL which
    li a7, \which
    ecall
    nop
.endm

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero
  csrr t0, CSR_MHARTID
  bnez t0, CORE2_EXCEP
  li a0, 0x0
  SBI_CALL SBI_EXT_0_1_SEND_IPI
  csrw CSR_SIP, zero
CORE2_EXCEP:
  la t0, exception_handler_entry
  csrw stvec, t0
  not t0, zero
  csrw sie, t0
  li t0, SR_SIE | SR_SUM
  csrw sstatus, t0

  jr ra
ENDPROC(setup_exception)
