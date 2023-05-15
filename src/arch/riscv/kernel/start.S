/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm/asm.h>
#include <asm/csr.h>

.section ".entry_function","ax"
ENTRY(_start)
    /* Mask all interrupts */
    csrw CSR_SIE, zero
    csrw CSR_SIP, zero

    /*
     * Disable FPU to detect illegal usage of
     * floating point in kernel space
     */
    li t0, SR_FS
    csrc sstatus, t0

    /* setup C environment */
    //lw sp, pid0_stack
    csrw CSR_SSCRATCH, x0

    /* Load the global pointer */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    csrr t0, CSR_MHARTID
    bnez t0, CORE2

    //these instructions will modify memory
    /* Clear BSS for flat non-ELF images */
    la a3, __bss_start
    la a4, __BSS_END__
    ble a4, a3, clear_bss_done
clear_bss:
    sd zero, (a3)
    add a3, a3, RISCV_SZPTR
    blt a3, a4, clear_bss
clear_bss_done:

    la a0, riscv_dtb
    sd a1, (a0)
    la tp, pid0_pcb
    li sp, 0xffffffc051001000
        //addi sp, sp, %lo(0xffffffc051001000)
    call main
CORE2:
    la tp, pid0_pcb2
    //lui sp, %hi(0xffffffc051002000)
        li sp, 0xffffffc051002000
    call main

loop:
    wfi
    j loop

END(_start)
