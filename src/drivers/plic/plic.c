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
    // set desired IRQ priorities non-zero (otherwise disabled).
    plic_base = (uintptr_t)ioremap((uint64_t)PLIC, 0x4000 * NORMAL_PAGE_SIZE);
    *(uint32 *)(plic_base + VIRTIO0_IRQ * 4) = 1;
}

void plic_init_hart(void) {
    // set enable bits for this hart's S-mode
    // for the uart and virtio disk.
    *(uint32 *)PLIC_SENABLE(0) = (1 << VIRTIO0_IRQ);
    *(uint32 *)PLIC_SENABLE(1) = (1 << VIRTIO0_IRQ);

    // set this hart's S-mode priority threshold to 0.
    *(uint32 *)PLIC_SPRIORITY(0) = 0;
    *(uint32 *)PLIC_SPRIORITY(1) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void) {
    int hart = get_current_cpu_id();
    int irq = *(uint32 *)PLIC_SCLAIM(hart);
    return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(int irq) {
    int hart = get_current_cpu_id();
    *(uint32 *)PLIC_SCLAIM(hart) = irq;
}
