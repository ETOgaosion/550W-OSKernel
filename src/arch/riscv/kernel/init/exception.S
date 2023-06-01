#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/sbidef.h>

ENTRY(setup_exception)
  /* 
   * save user_exception_handler_entry into STVEC
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
  csrw sip, zero
  # la t0, user_exception_handler_entry
  # csrw stvec, t0
  # li t0, SIE_SEIE
  # csrw sie, t0
  # li t0, SR_SIE
  # csrw sstatus, t0

  ret
ENDPROC(setup_exception)
