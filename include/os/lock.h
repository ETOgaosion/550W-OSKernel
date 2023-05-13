/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
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

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_
#define LOCKS_NUM 10
#define MAX_MBOX_LENGTH 100

#include <os/list.h>

typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef struct spin_lock
{
    volatile lock_status_t status;
} spin_lock_t;

typedef struct mutex_lock
{
    spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

typedef struct semaphore
{
    int count;
    list_head block_queue;
} semaphore_t;

typedef struct barrier
{
    int count;
    int countinit;
    list_head block_queue;
} barrier_t;

typedef struct mbox
{
    char msg[MAX_MBOX_LENGTH+5];
    int len_now;
    int users;
    list_head send_queue;
    list_head recv_queue;
} mbox_t;

/* init lock */
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

int do_mutex_lock_init(int userid);
void do_mutex_lock_acquire(int id);
void do_mutex_lock_release(int id);

int do_semaphore_init(int *id, int val);
int do_semaphore_up(int *id);
int do_semaphore_down(int *id);
int do_semaphore_destroy(int *id);

int do_barrier_init(int *id, unsigned count);
int do_barrier_wait(int *id);
int do_barrier_destroy(int *id);

int do_mbox_open(char * name);
void do_mbox_close(int);
int do_mbox_send(int, char *, int);
int do_mbox_recv(int, char *, int);
int do_test_send(int, int);
int do_test_recv(int, int);

#endif
