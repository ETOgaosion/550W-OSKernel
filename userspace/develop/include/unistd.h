#pragma once

#include "stddef.h"

extern int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...);

int open(const char *, int);
int openat(int, const char *, int);

ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
int mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data);
int umount(const char *special);

int close(int);
pid_t getpid(void);
pid_t getppid(void);
int sched_yield(void);
void exit(int);
pid_t fork(void);
pid_t clone(int (*fn)(void *arg), void *arg, void *stack, size_t stack_size, unsigned long flags);

int exec(char *name, char *const argv[], char *const envp[]);
int execve(const char *, char *const[], char *const[]);
int waitpid(int, int *, int);
int64 get_time();
long sys_get_time(TimeVal *ts, int tz); // syscall ID: 169; tz 表示时区，这里无需考虑，始终为0; 返回值：正确返回 0，错误返回 -1。
int times(void *mytimes);
int sleep(unsigned long long);
int set_priority(int prio);
void *mmap(void *, size_t, int, int, int, off_t);
int munmap(void *start, size_t len);
int wait(int *);
int spawn(char *file);
int mailread(void *buf, int len);
int mailwrite(int pid, void *buf, int len);
void sys_kill(int pid);

void sys_poweroff();

int fstat(int fd, struct kstat *st);
long sys_linkat(int olddirfd, char *oldpath, int newdirfd, char *newpath, unsigned int flags);
long sys_unlinkat(int dirfd, char *path, unsigned int flags);
int link(char *old_path, char *new_path);
int unlink(char *path);
int uname(void *buf);
int time(unsigned long *tloc);
int brk(void *);

char *getcwd(char *, size_t);
int chdir(const char *);
int mkdir(const char *, mode_t);
int getdents(int fd, struct linux_dirent64 *dirp64, unsigned long len);
int pipe(int[2]);
int dup(int);
int dup2(int, int);

void screen_clear();
void move_cursor(int x, int y);

long sys_process_show();
void sys_breakpoint();