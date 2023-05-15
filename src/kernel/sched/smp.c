#include <os/smp.h>
#include <asm/sbi.h>
#include <asm/atomic.h>
#include <os/sched.h>
#include <os/lock.h>

spin_lock_t kernel_lock;

void smp_init()
{
    spin_lock_init(&kernel_lock);
}

void wakeup_other_hart()
{
    sbi_send_ipi(NULL); 
    __asm__ __volatile__ ("csrw sip, zero\n\t");
}

void lock_kernel()
{
    spin_lock_acquire(&kernel_lock);
}

void unlock_kernel()
{
    spin_lock_release(&kernel_lock);
}
