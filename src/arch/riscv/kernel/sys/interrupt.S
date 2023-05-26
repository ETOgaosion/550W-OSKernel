#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/regs.h>
#include <asm/sbidef.h>

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs sstatus, t0
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc sstatus, t0
ENDPROC(disable_interrupt)

ENTRY(enable_software_interrupt)
  li t0, SIE_SSIE
  csrs sie, t0
ENDPROC(enable_software_interrupt)

ENTRY(disable_software_interrupt)
  li t0, SIE_SSIE
  csrc sie, t0
ENDPROC(disable_software_interrupt)

ENTRY(enable_timer_interrupt)
  li t0, SIE_STIE
  csrs sie, t0
ENDPROC(enable_timer_interrupt)

ENTRY(disable_timer_interrupt)
  li t0, SIE_STIE
  csrc sie, t0
ENDPROC(disable_timer_interrupt)

ENTRY(enable_external_interrupt)
  li t0, SIE_SEIE
  csrs sie, t0
ENDPROC(enable_external_interrupt)

ENTRY(disable_external_interrupt)
  li t0, SIE_SEIE
  csrc sie, t0
ENDPROC(disable_external_interrupt)

ENTRY(wfi)
  wfi
ENDPROC(wfi)
