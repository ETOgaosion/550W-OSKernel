#include <asm/asm.h>
#include <asm/csr.h>

ENTRY(get_current_cpu_id)
  csrr a0, sscratch
  ret
ENDPROC(get_current_cpu_id)

