/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
#include <context.h>

#include <type.h>
#include <os/list.h>
//#include <os/mm.h>

#define NUM_MAX_TASK 16
#define UNBLOCK_FROM_QUEUE 0
#define UNBLOCK_THIS_PCB 1

/* used to save register infomation */
/*typedef struct regs_context
{
    reg_t regs[32];

    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

typedef struct switchto_context
{
    reg_t regs[14];
} switchto_context_t;
*/
typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;
    
    /* process id */
    pid_t pid;
    pid_t fpid;
    pid_t tid;
    pid_t father_pid;
    pid_t child_pid[5];
    int child_num;

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /* cursor position */
    int cursor_x;
    int cursor_y;
    int priority;
    int first;
    int locksum;
    int lockid[10];
    int threadsum;
    int threadid[10];
    int cpuid;
    int killed;
    uint64_t pgdir;
    int port;
} pcb_t;

typedef struct super_block
{
    int magic;
    int fs_start_sec;
    int size;
    int imap_sec_offset;
    int imap_sec_size;
    int smap_sec_offset;
    int smap_sec_size;
    int inode_sec_offset;
    int inode_sec_size;
    int data_sec_offset;
    int data_sec_size;
    int sector_used;
    int inode_used;
    int root_inode_offset;
} super_block_t;

typedef struct dir_entry
{
    char name[20];
    int inode_id;
    int last;
    int mode;
} dentry_t;

typedef struct inode_s
{
    //pointers need -1
    int sec_size;
    int mode;
    int direct_block_pointers[11];
    int indirect_block_pointers[3];
    int double_block_pointers[2];
    int trible_block_pointers;
    int link_num;
} inode_t;

typedef struct fentry
{
    int inodeid;
    int prive;
    int pos_block;
    int pos_offset;
} fentry_t;

extern inode_t nowinode;

extern super_block_t superblock;
extern super_block_t superblock2;

extern uint64_t imap_addr_offset;
extern uint64_t bmap_addr_offset;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
} task_info_t;

/* ready queue to run */
//extern list_head* ready_queue0;
//extern list_head* ready_queue1;
extern int cpu_lock;

/* current running task PCB */
extern pcb_t * volatile current_running0;
extern pcb_t * volatile current_running1;
extern pcb_t * volatile current_running;
extern list_head* ready_queue;
extern list_head* ready_queue0;
extern list_head* ready_queue1;
// extern pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;
extern int cpu_lock;
extern int allocpid;

extern list_node_t send_queue;
extern list_node_t recv_queue;

extern pcb_t pcb[NUM_MAX_TASK];
// extern pcb_t kernel_pcb[NR_CPUS];
extern pcb_t pid0_pcb;
extern pcb_t pid0_pcb2;
extern const ptr_t pid0_stack;
extern const ptr_t pid0_stack2;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);
void check_sleeping();
int check_send();
int check_recv();

void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *, int type);

extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();
 
#endif
