#include <asm/asm.h>
#include <asm/csr.h>
#include <asm/sbidef.h>
#include <asm/sbiasm.h>

ENTRY(setup_exception)
    /* TODO: 
     * save exception_handler_entry into STVEC
     * enable global exceptions */
    csrw CSR_SIE, zero
    csrw CSR_SIP, zero
    csrr t0, CSR_MHARTID
    bnez t0, CORE2_EXCEP
    li a0, 0x0
    SBI_CALL SBI_SEND_IPI
    csrw CSR_SIP, zero
CORE2_EXCEP:
    la t0, exception_handler_entry
    csrw CSR_STVEC, t0
    li t0, 0x222
    csrw CSR_SIE, t0
    li t0, SR_SIE
    csrw CSR_SSTATUS, t0

    jr ra
ENDPROC(setup_exception)
