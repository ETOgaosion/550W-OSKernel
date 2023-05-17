#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/smp.h>

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main() {
    int id = get_current_cpu_id();
    if (id == 1) {
        setup_exception();
        cancelpg(pa2kva(PGDIR_PA));
        sbi_set_timer(0);
    }
    // init Process Control Block (-_-!)
    init_pcb();
    // printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // init system call table (0_0)
    init_syscall();
    // printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);
    init_exception();
    // printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init screen (QAQ)
    init_screen();
    // printk("> [INIT] SCREEN initialization succeeded.\n\r");
    // Setup timer interrupt and enable all interrupt
    // init interrupt (^_^)
    setup_exception();
    sbi_set_timer(0);
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control sys_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        // sys_scheduler();
    };
    return 0;
}
