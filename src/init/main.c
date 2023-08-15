#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/privileged.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <asm/vm.h>
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
#include <os/resources.h>
#include <os/smp.h>
#include <os/socket.h>
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

int kernel_start(int mhartid) {
    int id = mhartid;
    if (id != 0) {
        asm_w_stvec((uint64_t)kernel_exception_handler_entry);

        k_smp_lock_kernel();

        current_running = &current_running1;

        k_smp_set_current_pcb(current_running1);

        d_plic_init_hart(id);
    } else {
        // init kernel lock
        k_smp_init();

        k_mm_init_mm();

        asm_w_stvec((uint64_t)kernel_exception_handler_entry);

        k_smp_lock_kernel();

        // init Process Control Block (-_-!)
        k_pcb_init();

        k_smp_set_current_pcb(current_running0);

        k_sys_write_to_log("=============== dmesg ===============\n\r");

        k_print("[INIT] > PCB initialization succeeded.\n\r");
        k_sys_write_to_log("[INIT] > PCB initialization succeeded.\n\r");

        k_sysinfo_init();

        k_resources_init();

        // read CPU frequency
        k_time_init();

        // init system call table (0_0)
        k_syscall_init();
        k_print("[INIT] > System call initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > System call initialized successfully.\n\r");

        k_exception_init();
        k_signal_init_sig_table();

        // init disk
        d_plic_init();
        d_plic_init_hart(id);
        d_virtio_disk_init();
        d_binit();
        k_print("[INIT] > Disk initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > Disk initialized successfully.\n\r");

        // init screen
        d_screen_init();

        // init fs
        fs_init();
        fd_table_init();
        k_print("[INIT] > fs initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > fs initialized successfully.\n\r");

        // init socket
        k_socket_init();
        k_print("[INIT] > socket initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > socket initialized successfully.\n\r");

        // init users
        k_users_init();
        k_print("[INIT] > users initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > users initialized successfully.\n\r");

        // init built-in tasks
        spawn("shell");
        spawn("bubble");
        spawn("bubble");
        k_print("[INIT] > daemons initialized successfully.\n\r");
        k_sys_write_to_log("[INIT] > daemons initialized successfully.\n\r");

        wakeup_other_cores();
    }
    // init sbi timer, all cores shall done
    sbi_set_timer(0);
    // init exceptions
    setup_exception();
    // start scheduling
    while (1) {
        k_pcb_scheduler(false, true);
        k_smp_unlock_kernel();
        k_smp_lock_kernel();
        k_smp_sync_current_pcb();
    };
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main(unsigned long mhartid) {
    prepare_vm(mhartid);
    return kernel_start(mhartid);
}
