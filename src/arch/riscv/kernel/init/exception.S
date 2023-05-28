#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/sbidef.h>

ENTRY(setup_exception)
  /* 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  csrr t0, sscratch
  bnez t0, CORE2_EXCEP
  la tp, current_running0
  ld tp, (tp)
  j NEXT
CORE2_EXCEP:
  la tp, current_running1
  ld tp, (tp)
NEXT:
  csrw CSR_SIP, zero
  la t0, exception_handler_entry
  csrw CSR_STVEC, t0
  li t0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw CSR_SIE, t0
  li t0, SR_SIE
  csrw CSR_SSTATUS, t0

  jr ra
ENDPROC(setup_exception)
