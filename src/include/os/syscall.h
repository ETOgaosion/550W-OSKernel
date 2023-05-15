#pragma once

#include <common/type.h>
#include <uapi/syscall_number.h>
#include <os/sched.h>

#define SCREEN_HEIGHT 80
// pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t
// mode);
pid_t sys_exec(const char *file_name, int argc, char *argv);
void sys_show_exec();

long invoke_syscall(long, long, long, long, long);

void sys_sleep(uint32_t);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

long sys_get_timebase();
long sys_get_tick();

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode);
void sys_exit(void);
int sys_kill(pid_t pid);
int sys_waitpid(pid_t pid);
void sys_process_show(void);
void sys_screen_clear(void);
pid_t sys_getpid();
int sys_get_char();
uint64_t sys_shmpageget(int);
void sys_shmpagedt(uint64_t);

#define BINSEM_OP_LOCK 0   // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

int binsemget(int key);
int binsemop(int binsem_id, int op);
