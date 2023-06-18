#include <asm/privileged.h>
#include <asm/sbi.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/smp.h>

spin_lock_t kernel_lock;

void k_smp_init() {
    k_spin_lock_init(&kernel_lock);
}

void k_smp_wakeup_other_hart() {
    sbi_send_ipi(NULL);
    __asm__ __volatile__("csrw sip, zero\n\t");
}

extern void kernel_exception_handler_entry();

void k_smp_lock_kernel() {
    asm_w_stvec((uint64_t)kernel_exception_handler_entry);
    disable_timer_interrupt();
    disable_software_interrupt();
    enable_external_interrupt();
    enable_interrupt();
    k_spin_lock_acquire(&kernel_lock);
    current_running = k_smp_get_current_running();
}

void k_smp_unlock_kernel() {
    k_spin_lock_release(&kernel_lock);
}

pcb_t *volatile *k_smp_get_current_running() {
    return k_smp_get_current_cpu_id() ? &current_running1 : &current_running0;
}
