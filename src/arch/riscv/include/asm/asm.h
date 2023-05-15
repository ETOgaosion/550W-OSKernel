#pragma once

#define END(function)  \
       .size function, .- function

#define ENDPROC(name)                           \
        .type name, @function;                  \
        END(name)

#define EXPORT(symbol) \
        .globl symbol; \
        symbol:

#define FEXPORT(symbol)              \
        .globl symbol;               \
        .type symbol, @function;     \
        symbol:

#define ENTRY(name)                             \
        .globl name;                            \
        .balign 4;                              \
        name:

#define RISCV_PTR		.dword
#define RISCV_SZPTR		8
#define RISCV_LGPTR		3
