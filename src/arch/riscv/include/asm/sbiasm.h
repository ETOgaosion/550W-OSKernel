#pragma once

/* clang-format off */
.macro SBI_CALL which
    li a7, \which
    ecall
    nop
.endm
                             /* clang-format on */
