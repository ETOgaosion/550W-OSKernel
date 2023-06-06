#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/privileged.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <drivers/plic/plic.h>
#include <drivers/screen/screen.h>
#include <drivers/virtio/virtio.h>
#include <fs/fs.h>
#include <fs/file.h>
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

extern void kernel_exception_handler_entry();

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main() {
    int id = get_current_cpu_id();
    if (id != 0) {
        w_stvec((uint64_t)kernel_exception_handler_entry);
        k_lock_kernel();
        plic_init_hart();
        current_running = k_get_current_running();
        // k_cancel_pg(pa2kva(PGDIR_PA));
    } else {
        k_smp_init(); // only done by master core
        w_stvec((uint64_t)kernel_exception_handler_entry);
        k_lock_kernel();
        // init fs
        // init_fs();

        // init Process Control Block (-_-!)
        k_init_pcb();

        printk("> [INIT] PCB initialization succeeded.\n\r");

        // read CPU frequency
        time_base = TIME_BASE_DEFAULT;

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // init screen (QAQ)
        init_exception();

        plic_init();
        plic_init_hart();
        virtio_disk_init();
        binit();
        printk("> [INIT] Disk initialized successfully.\n\r");

        init_screen();
        // Setup timer interrupt and enable all interrupt
        // init interrupt (^_^)-
        fat32_init();
        // fd_table_init();
        sys_spawn("shell");

        wakeup_other_cores();
    }
    sbi_set_timer(0);
    setup_exception();
    while (1) {
        k_scheduler();
    };
    return 0;
}
