#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/syscall_number.h>

int mthread_mutex_init(void* handle)
{
    /* TODO: */
    invoke_syscall(SYSCALL_LOCK_INIT, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}
int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    invoke_syscall(SYSCALL_LOCK_ACQUIRE, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}
int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    invoke_syscall(SYSCALL_LOCK_RELEASE, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_INIT, handle, count, IGNORE, IGNORE);
    return 1;
}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_WAIT, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_DESTROY, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, handle, val, IGNORE, IGNORE);
    return 1;
}
int mthread_semaphore_up(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_UP, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}
int mthread_semaphore_down(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}
int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DESTROY, handle, IGNORE, IGNORE, IGNORE);
    return 1;
}

int binsemget(int key)
{
    return invoke_syscall(SYSCALL_LOCK_INIT, key, IGNORE, IGNORE, IGNORE);
}
int binsemop(int binsem_id, int op)
{
    if(op == 0)
    {
        invoke_syscall(SYSCALL_LOCK_ACQUIRE, binsem_id, IGNORE, IGNORE, IGNORE);
        return 1;
    }else if(op==1)
    {
        invoke_syscall(SYSCALL_LOCK_RELEASE, binsem_id, IGNORE, IGNORE, IGNORE);
        return 1;
    }else
        return 0;
}

int mthread_create(mthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    return invoke_syscall(SYSCALL_MTHREAD_CREATE, thread, start_routine, arg, IGNORE);
}

int mthread_join(mthread_t thread)
{
    return invoke_syscall(SYSCALL_WAITPID, thread, IGNORE, IGNORE, IGNORE);
}
