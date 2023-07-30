#pragma once

#include <asm/sbidef.h>
#include <common/types.h>

/* clang-format off */

#define SBI_CALL(which, arg0, arg1, arg2)                     \
    ({                                                        \
        register uintptr_t a0 asm("a0") = (uintptr_t)(arg0);  \
        register uintptr_t a1 asm("a1") = (uintptr_t)(arg1);  \
        register uintptr_t a2 asm("a2") = (uintptr_t)(arg2);  \
        register uintptr_t a6 asm("a6") = (uintptr_t)(0);     \
        register uintptr_t a7 asm("a7") = (uintptr_t)(which); \
        asm volatile("ecall"                                  \
                     : "+r"(a0)                               \
                     : "r"(a1), "r"(a2), "r"(a6), "r"(a7)              \
                     : "memory");                             \
        a0;                                                   \
    })

/* clang-format on */

/* Lazy implementations until SBI is finalized */
#define SBI_CALL_0(which) SBI_CALL(which, 0, 0, 0)
#define SBI_CALL_1(which, arg0) SBI_CALL(which, arg0, 0, 0)
#define SBI_CALL_2(which, arg0, arg1) SBI_CALL(which, arg0, arg1, 0)
#define SBI_CALL_3(which, arg0, arg1, arg2) SBI_CALL(which, arg0, arg1, arg2)

static inline void sbi_console_putstr(char *str) {
    while (*str != '\0') {
        SBI_CALL_1(SBI_EXT_0_1_CONSOLE_PUTCHAR, *str++);
    }
}

static inline void sbi_console_putchar(int ch) {
    SBI_CALL_1(SBI_EXT_0_1_CONSOLE_PUTCHAR, ch);
}

static inline int sbi_console_getchar(void) {
    return SBI_CALL_0(SBI_EXT_0_1_CONSOLE_GETCHAR);
}

static inline void sbi_set_timer(uint64_t stime_value) {
    SBI_CALL_1(SBI_EXT_TIME, stime_value);
}

static inline void sbi_set_mode(uint64_t hart_id, uint64_t addr) {
    SBI_CALL_2(SBI_EXT_HSM, hart_id, addr);
}

static inline void sbi_shutdown(void) {
    SBI_CALL_0(SBI_EXT_0_1_SHUTDOWN);
}

static inline void sbi_clear_ipi(void) {
    SBI_CALL_0(SBI_EXT_0_1_CLEAR_IPI);
}

static inline void sbi_send_ipi(const unsigned long *hart_mask) {
    SBI_CALL_1(SBI_EXT_0_1_SEND_IPI, hart_mask);
}

static inline void sbi_remote_fence_i(const unsigned long *hart_mask) {
    SBI_CALL_1(SBI_EXT_0_1_REMOTE_FENCE_I, hart_mask);
}

static inline void sbi_remote_sfence_vma(const unsigned long *hart_mask, unsigned long start, unsigned long size) {
    SBI_CALL_1(SBI_EXT_0_1_REMOTE_SFENCE_VMA, hart_mask);
}

static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask, unsigned long start, unsigned long size, unsigned long asid) {
    SBI_CALL_1(SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID, hart_mask);
}