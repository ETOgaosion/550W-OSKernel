#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <drivers/plic/plic.h>
#include <drivers/screen/screen.h>
#include <drivers/virtio/virtio.h>
#include <fs/fs.h>
#include <lib/stdio.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <os/time.h>

void wakeup_other_cores() {
    int hartid = 1;
    uint64_t mask = 1 << hartid;
    sbi_set_mode(hartid, 0x80200000);
    sbi_send_ipi(&mask);
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main() {
    int id = get_current_cpu_id();
    if (id == 1) {
        current_running = get_current_running();
        setup_exception();
        // cancelpg(pa2kva(PGDIR_PA));
        sbi_set_timer(0);
    }

    // init fs
    // init_fs();

    // init Process Control Block (-_-!)

    k_init_pcb();

    // printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = TIME_BASE_DEFAULT;

    // init system call table (0_0)
    init_syscall();
    // printk("> [INIT] System call initialized successfully.\n\r");
    // init screen (QAQ)
    init_screen();

    // fdt_print(riscv_dtb);
    // printk("> [INIT] Interrupt processing initialization succeeded.\n\r");
    plic_init();
    plic_init_hart();
    binit();
    virtio_disk_init();
    return 0;

    // printk("> [INIT] SCREEN initialization succeeded.\n\r");
    // Setup timer interrupt and enable all interrupt
    // init interrupt (^_^)
    wakeup_other_cores();
    setup_exception();
    sbi_set_timer(0);
    while (1) {
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        // k_scheduler();
    };
    return 0;
}
