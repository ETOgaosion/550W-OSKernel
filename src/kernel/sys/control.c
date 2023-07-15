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

int restart_flg = 0;

int kernel_restart() {
    int id = k_smp_get_current_cpu_id();
    if (id != 0) {
        while (restart_flg == 0) {
            k_smp_unlock_kernel();
            k_smp_lock_kernel();
        }
        current_running = k_smp_get_current_running();
    } else {
        // init kernel lock
        // init Process Control Block (-_-!)
        k_pcb_init();
        k_print("> [INIT] PCB initialization succeeded.\n\r");

        // init screen
        d_screen_init();

        // init fs
        fs_init();
        fd_table_init();

        // init built-in tasks
        sys_spawn("shell");
        sys_spawn("bubble");
        sys_spawn("bubble");

        restart_flg = 1;
    }
    // init sbi timer, all cores shall done
    sbi_set_timer(0);
    // init exceptions
    setup_exception();
    // start scheduling
    while (1) {
        k_pcb_scheduler(false);
        k_smp_unlock_kernel();
        k_smp_lock_kernel();
    };
}

// [TODO]
long sys_reboot(int magic1, int magic2, unsigned int cmd, void *arg) {
    if (magic1 != MOSS_REBOOT_MAGIC1 || (magic2 != MOSS_REBOOT_MAGIC2 && magic2 != MOSS_REBOOT_MAGIC2A && magic2 != MOSS_REBOOT_MAGIC2B && magic2 != MOSS_REBOOT_MAGIC2C)) {
        return -EINVAL;
    }
    switch (cmd) {
    case MOSS_REBOOT_CMD_RESTART:
        kernel_restart();
    default:
        break;
    }
    return 0;
}