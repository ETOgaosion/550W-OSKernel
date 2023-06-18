#ifndef IRQ_H
#define IRQ_H

#include <common/types.h>
#include <os/pcb.h>

#define PLIC_NR_IRQS 32
#define PLIC_NR_CTXS 4
#define TIMER_INTERVAL 150000

/* ERROR code */
enum IrqCode {
    IRQC_U_SOFT = 0,
    IRQC_S_SOFT = 1,
    IRQC_M_SOFT = 3,
    IRQC_U_TIMER = 4,
    IRQC_S_TIMER = 5,
    IRQC_M_TIMER = 7,
    IRQC_U_EXT = 8,
    IRQC_S_EXT = 9,
    IRQC_M_EXT = 11,
    IRQC_COUNT
};

enum ExcCode {
    EXCC_INST_MISALIGNED = 0,
    EXCC_INST_ACCESS = 1,
    EXCC_BREAKPOINT = 3,
    EXCC_LOAD_ACCESS = 5,
    EXCC_STORE_ACCESS = 7,
    EXCC_SYSCALL = 8,
    EXCC_INST_PAGE_FAULT = 12,
    EXCC_LOAD_PAGE_FAULT = 13,
    EXCC_STORE_PAGE_FAULT = 15,
    EXCC_COUNT
};

enum IrqExtCode {
    IRQC_EXT_VIRTIO_BLK_IEQ = 1,
    IRQC_EXT_COUNT
};

typedef void (*handler_t)(regs_context_t *, uint64_t, uint64_t, uint64_t);

void k_init_syscall(void);

void k_init_exception();
void setup_exception();
void enable_interrupt();
void disable_interrupt();
void enable_software_interrupt();
void disable_software_interrupt();
void enable_timer_interrupt();
void disable_timer_interrupt();
void enable_external_interrupt();
void disable_external_interrupt();
void wfi();

#endif
