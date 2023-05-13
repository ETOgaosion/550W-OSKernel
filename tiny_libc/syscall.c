#include <sys/syscall.h>
#include <stdint.h>
#include <os/syscall_number.h>
#include <os.h>

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
    
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, buff, IGNORE, IGNORE, IGNORE);
    
}

int sys_read()
{
    // TODO:
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
    
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
    
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
    
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
    // TODO:
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
    // TODO:
}

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t* time_elap)
{
    return invoke_syscall(SYSCALL_GET_WALL_TIME, time_elap, IGNORE, IGNORE, IGNORE);
}

int sys_ps()
{
    return invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_clear()
{
    invoke_syscall(SYSCALL_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

int sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_taskset(int pid, int mask)
{
    return invoke_syscall(SYSCALL_TASKSET, pid, mask, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv)
{
    return invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, IGNORE);
}

int sys_fs()
{
    return invoke_syscall(SYSCALL_FS, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_fork()
{
    return invoke_syscall(SYSCALL_FORK, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint64_t sys_getpa(uint64_t va)
{
    return invoke_syscall(SYSCALL_GETPA, va, IGNORE, IGNORE, IGNORE);
}

uint64_t sys_shmpageget(int key)
{
    return invoke_syscall(SYSCALL_SHMPAGE_GET, key, IGNORE, IGNORE, IGNORE);
}

void sys_shmpagedt(uint64_t vars)
{
    invoke_syscall(SYSCALL_SHMPAGE_DT, vars, IGNORE, IGNORE, IGNORE);
} 

long sys_net_recv(uintptr_t addr, int length, int num_packet, void* frLength)
{
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}

void sys_net_send(uintptr_t addr, int length)
{
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}

void sys_net_irq_mode(int mode)
{
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}

void sys_net_port(int mode)
{
    invoke_syscall(SYSCALL_NET_PORT, mode, IGNORE, IGNORE, IGNORE);
}

int sys_mkfs()
{
    return invoke_syscall(SYSCALL_MKFS, 1, IGNORE, IGNORE, IGNORE);
}

int sys_statfs()
{
    return invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_ls(char* name, int func)
{
    return invoke_syscall(SYSCALL_LS, name, func, IGNORE, IGNORE);
}

int sys_rmdir(char* name)
{
    return invoke_syscall(SYSCALL_RMDIR, name, IGNORE, IGNORE, IGNORE);
}

int sys_mkdir(char* name)
{
    return invoke_syscall(SYSCALL_MKDIR, name, IGNORE, IGNORE, IGNORE);
}

int sys_cd(char* name)
{
    return invoke_syscall(SYSCALL_CD, name, IGNORE, IGNORE, IGNORE);
}

int sys_touch(char* name)
{
    return invoke_syscall(SYSCALL_TOUCH, name, IGNORE, IGNORE, IGNORE);
}

int sys_cat(char* name)
{
    return invoke_syscall(SYSCALL_CAT, name, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char* name, int access)
{
    return invoke_syscall(SYSCALL_FOPEN, name, access, IGNORE, IGNORE);
}

int sys_fread(int fd, char* buff, int size)
{
    return invoke_syscall(SYSCALL_FREAD, fd, buff, size, IGNORE);
}

int sys_fwrite(int fd, char* buff, int size)
{
    return invoke_syscall(SYSCALL_FWRITE, fd, buff, size, IGNORE);
}

int sys_close(int fd)
{
    return invoke_syscall(SYSCALL_FCLOSE, fd, IGNORE, IGNORE, IGNORE);
}

int sys_rm(char* name)
{
    return invoke_syscall(SYSCALL_RM, name, IGNORE, IGNORE, IGNORE);
}

int sys_ln(char* name, char *path)
{
    return invoke_syscall(SYSCALL_LN, name, path, IGNORE, IGNORE);
}

int sys_lseek(int fid, int offset, int whence)
{
    return invoke_syscall(SYSCALL_LSEEK, fid, offset, whence, IGNORE);
}