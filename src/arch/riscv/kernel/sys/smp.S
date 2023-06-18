#include <asm/asm.h>
#include <asm/csr.h>

ENTRY(k_smp_get_current_cpu_id)
  csrr a0, sscratch
  ret
ENDPROC(k_smp_get_current_cpu_id)

