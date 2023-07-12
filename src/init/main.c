#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/privileged.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <drivers/plic/plic.h>
#include <drivers/screen/screen.h>
#include <drivers/virtio/virtio.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <lib/stdio.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <os/sys.h>
#include <os/time.h>
#include <os/users.h>

void wakeup_other_cores() {
    int hartid = 1;
    uint64_t mask = 1 << hartid;
    sbi_set_mode(hartid, 0x80200000);
    sbi_send_ipi(&mask);
}

extern void kernel_exception_handler_entry();

int kernel_start() {
    int id = k_smp_get_current_cpu_id();
    if (id != 0) {
        asm_w_stvec((uint64_t)kernel_exception_handler_entry);

        k_smp_lock_kernel();

        d_plic_init_hart();

        current_running = k_smp_get_current_running();
    } else {
        // init kernel lock
        k_smp_init();

        asm_w_stvec((uint64_t)kernel_exception_handler_entry);

        k_smp_lock_kernel();

        // init Process Control Block (-_-!)
        k_pcb_init();

        k_print("> [INIT] PCB initialization succeeded.\n\r");

        // read CPU frequency
        time_base = TIME_BASE_DEFAULT;

        // init system call table (0_0)
        k_init_syscall();
        k_print("> [INIT] System call initialized successfully.\n\r");

        // init screen (QAQ)
        k_init_exception();

        // init disk
        d_plic_init();
        d_plic_init_hart();
        d_virtio_disk_init();
        d_binit();
        k_print("> [INIT] Disk initialized successfully.\n\r");

        // init screen
        d_screen_init();

        // init fs
        fs_init();
        fd_table_init();

        // init users
        init_users();

        // init built-in tasks
        sys_spawn("shell");
        sys_spawn("bubble");
        sys_spawn("bubble");

        wakeup_other_cores();
    }
    // init sbi timer, all cores shall done
    sbi_set_timer(0);
    // init exceptions
    setup_exception();
    // start scheduling
    while (1) {
        k_pcb_scheduler();
        k_smp_unlock_kernel();
        k_smp_lock_kernel();
    };
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main() {
    return kernel_start();
}
