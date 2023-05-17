#pragma once

#include <common/types.h>
#include <os/sched.h>

// pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t
// mode);
extern pid_t sys_exec(const char *file_name, int argc, char **argv);
extern void sys_show_exec();

extern long invoke_syscall(long, long, long, long, long);
extern void sys_undefined_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause);

extern void sys_scheduler(void);
extern void sys_sleep(uint32_t);
extern int sys_taskset(int pid, int mask);

extern void sys_write(char *);
extern void sys_move_cursor(int, int);
extern void sys_reflush();

extern long sys_get_timebase();
extern long sys_get_tick();

extern pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode);
extern void sys_exit(void);
extern int sys_kill(pid_t pid);
extern int sys_waitpid(pid_t pid);
extern int sys_process_show();
extern void sys_screen_clear(void);

extern int sys_mthread_create(pid_t *thread, void (*start_routine)(void *), void *arg);
extern int sys_fork(int prior);

extern int sys_port_read();
void sys_screen_clear(void);
void sys_screen_reflush(void);
void sys_screen_write(char *buff);
void sys_screen_move_cursor(int x, int y);

extern uint64_t sys_get_ticks(void);
uint64_t sys_get_time_base();

extern int sys_mutex_lock_init(int userid);
extern void sys_mutex_lock_acquire(int id);
extern void sys_mutex_lock_release(int id);

extern int sys_semaphore_init(int *id, int val);
extern int sys_semaphore_up(int *id);
extern int sys_semaphore_down(int *id);
extern int sys_semaphore_destroy(int *id);

extern int sys_barrier_init(int *id, unsigned count);
extern int sys_barrier_wait(int *id);
extern int sys_barrier_destroy(int *id);

extern int sys_mbox_open(const char *name);
extern void sys_mbox_close(int);
extern int sys_mbox_send(int, char *, int);
extern int sys_mbox_recv(int, char *, int);
extern int sys_test_send(int, int);
extern int sys_test_recv(int, int);

extern uintptr_t sys_shm_page_get(int key);
extern void sys_shm_page_dt(uintptr_t addr);

extern int sys_mkfs(int func);
extern int sys_statfs();
extern int sys_ls(const char *name, int func);
extern int sys_rmdir(const char *name);
extern int sys_mkdir(const char *name);
extern int sys_cd(const char *name);
extern int sys_touch(const char *name);
extern int sys_cat(const char *name);
extern int sys_fopen(const char *name, int access);
extern int sys_fread(int fid, char *buff, int size);
extern int sys_fwrite(int fid, char *buff, int size);
extern void sys_fclose(int fid);
extern int sys_ln(const char *name, char *path);
extern int sys_rm(const char *name);
extern int sys_lseek(int fid, int offset, int whence);