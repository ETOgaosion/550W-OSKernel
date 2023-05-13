#include <asm.h>
#include <csr.h>

ENTRY(invoke_syscall)
  /* TODO: */
  addi a7, a0, 0
  addi a0, a1, 0
  addi a1, a2, 0
  addi a2, a3, 0
  addi a3, a4, 0
  ecall
  jr ra
ENDPROC(invoke_syscall)

/*ENTRY(sys_fork)
  li a7, 11 // SYSCALL_FORK
  //a0
  li a1, 0
  li a2, 0
  ecall
  jr ra
ENDPROC(sys_fork)
*/