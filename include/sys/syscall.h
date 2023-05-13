/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
#include <stdint.h>
#include <os.h>
 
#define SCREEN_HEIGHT 80
//pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
pid_t sys_exec(const char *file_name, int argc, char* argv);
void sys_show_exec();

extern long invoke_syscall(long, long, long, long, long);

void sys_sleep(uint32_t);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

long sys_get_timebase();
long sys_get_tick();

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode);
void sys_exit(void);
int sys_kill(pid_t pid);
int sys_waitpid(pid_t pid);
void sys_process_show(void);
void sys_screen_clear(void);
pid_t sys_getpid();
int sys_get_char();
uint64_t sys_shmpageget(int);
void sys_shmpagedt(uint64_t);

#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release
//long sys_net_recv(uintptr_t addr, void* length, int num_packet, void* frLength);
//void sys_net_send(uintptr_t addr, void* length);
void sys_net_irq_mode(int mode);
extern void sys_net_port(int mode);

int binsemget(int key);
int binsemop(int binsem_id, int op);
#endif
