#include <asm/asm.h>
#include <asm/csr.h>

ENTRY(get_current_cpu_id)
  csrr a0, sscratch
  jr ra
ENDPROC(get_current_cpu_id)

