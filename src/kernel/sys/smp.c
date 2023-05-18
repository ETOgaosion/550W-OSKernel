#include <asm/sbi.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/smp.h>

spin_lock_t kernel_lock;

void smp_init() { spin_lock_init(&kernel_lock); }

void wakeup_other_hart() {
    sbi_send_ipi(NULL);
    __asm__ __volatile__("csrw sip, zero\n\t");
}

void lock_kernel() { spin_lock_acquire(&kernel_lock); }

void unlock_kernel() { spin_lock_release(&kernel_lock); }

pcb_t *get_current_running() { return get_current_cpu_id() ? current_running1 : current_running0; }
