#include <common/types.h>
#include <drivers/plic/plic.h>
#include <drivers/virtio/virtio.h>
#include <os/ioremap.h>
#include <os/smp.h>

#define PLIC 0x0c000000L

uintptr_t plic_base;

#define PLIC_PRIORITY (plic_base + 0x0)
#define PLIC_PENDING (plic_base + 0x1000)
#define PLIC_MENABLE(hart) (plic_base + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (plic_base + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (plic_base + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (plic_base + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (plic_base + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (plic_base + 0x201004 + (hart)*0x2000)

void plic_init(void) {
    plic_base = (uintptr_t)ioremap((uint64_t)PLIC, 0x4000 * NORMAL_PAGE_SIZE);
    writed(1, plic_base + DISK_IRQ * sizeof(uint32));
}

void plic_init_hart(void) {
    int hart = get_current_cpu_id();
    // set uart's enable bit for this hart's S-mode.
    *(uint32 *)PLIC_SENABLE(hart) = (1 << UART_IRQ) | (1 << DISK_IRQ);
    // set this hart's S-mode priority threshold to 0.
    *(uint32 *)PLIC_SPRIORITY(hart) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void) {
    int hart = get_current_cpu_id();
    int irq;
    irq = *(uint32 *)PLIC_SCLAIM(hart);
    return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(int irq) {
    int hart = get_current_cpu_id();
    *(uint32 *)PLIC_SCLAIM(hart) = irq;
}