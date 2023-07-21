#include <asm/asm.h>
#include <asm/csr.h>

ENTRY(k_smp_set_current_pcb)
  mv tp, a0
  jr ra
ENDPROC(k_smp_set_current_pcb)

ENTRY(k_smp_get_current_pcb)
  mv a0, tp
  jr ra
ENDPROC(k_smp_get_current_pcb)

