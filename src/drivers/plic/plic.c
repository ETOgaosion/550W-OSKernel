#include <common/types.h>
#include <drivers/plic/plic.h>
#include <drivers/virtio/virtio.h>
#include <os/smp.h>

void plic_init(void) {
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32 *)(PLIC + VIRTIO0_IRQ * 4) = 1;
}

void plic_init_hart(void) {
    int hart = get_current_cpu_id();

    // set enable bits for this hart's S-mode
    // for the uart and virtio disk.
    *(uint32 *)PLIC_SENABLE(hart) = (1 << VIRTIO0_IRQ);

    // set this hart's S-mode priority threshold to 0.
    *(uint32 *)PLIC_SPRIORITY(hart) = 0;
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
