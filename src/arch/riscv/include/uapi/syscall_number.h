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

#ifndef OS_SYSCALL_NUMBER_H_
#define OS_SYSCALL_NUMBER_H_

#define IGNORE 0
#define NUM_SYSCALLS 128

/* define */
#define SYSCALL_SPAWN 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7

#define SYSCALL_FUTEX_WAIT 10
#define SYSCALL_FUTEX_WAKEUP 11

#define SYSCALL_LOCK_INIT 12
#define SYSCALL_LOCK_ACQUIRE 13
#define SYSCALL_LOCK_RELEASE 14
#define SYSCALL_FORK 15
#define SYSCALL_GET_WALL_TIME 16
#define SYSCALL_CLEAR 17

#define SYSCALL_WRITE 20
#define SYSCALL_READ 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_SERIAL_READ 24
#define SYSCALL_SERIAL_WRITE 25
#define SYSCALL_READ_SHELL_BUFF 26
#define SYSCALL_SCREEN_CLEAR 27

#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_GET_CHAR 32

#define SYSCALL_SEMAPHORE_INIT 33
#define SYSCALL_SEMAPHORE_UP 34
#define SYSCALL_SEMAPHORE_DOWN 35
#define SYSCALL_SEMAPHORE_DESTROY 36
#define SYSCALL_BARRIER_INIT 37
#define SYSCALL_BARRIER_WAIT 38
#define SYSCALL_BARRIER_DESTROY 39
#define SYSCALL_MBOX_OPEN 40
#define SYSCALL_MBOX_CLOSE 41
#define SYSCALL_MBOX_SEND 42
#define SYSCALL_MBOX_RECV 43
#define SYSCALL_TASKSET 44
#define SYSCALL_TEST_SEND 45
#define SYSCALL_TEST_RECV 46
#define SYSCALL_EXEC 47
#define SYSCALL_FS 48
#define SYSCALL_MTHREAD_CREATE 49
#define SYSCALL_MTHREAD_JOIN 50
#define SYSCALL_GETPA 51
#define SYSCALL_SHMPAGE_GET 52
#define SYSCALL_SHMPAGE_DT 53
#define SYSCALL_NET_RECV 54
#define SYSCALL_NET_SEND 55
#define SYSCALL_NET_IRQ_MODE 56
#define SYSCALL_NET_PORT 57

#define SYSCALL_MKFS 58
#define SYSCALL_STATFS 59
#define SYSCALL_CD 60
#define SYSCALL_MKDIR 61
#define SYSCALL_RMDIR 62
#define SYSCALL_LS 63

#define SYSCALL_TOUCH 64
#define SYSCALL_CAT 65
#define SYSCALL_FOPEN 66
#define SYSCALL_FREAD 67
#define SYSCALL_FWRITE 68
#define SYSCALL_FCLOSE 69

#define SYSCALL_LN 70
#define SYSCALL_RM 71
#define SYSCALL_LSEEK 72

#endif
