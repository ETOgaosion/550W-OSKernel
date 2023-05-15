#pragma once

#include <asm/context.h>
#include <common/type.h>
#include <lib/list.h>
#include <os/mm.h>

#define NUM_MAX_TASK 16
#define UNBLOCK_FROM_QUEUE 0
#define UNBLOCK_THIS_PCB 1

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
typedef struct pcb {
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

/* task information, used to init PCB */
typedef struct task_info {
    ptr_t entry_point;
    task_type_t type;
} task_info_t;

/* ready queue to run */
// list_head* ready_queue0;
// list_head* ready_queue1;

/* current running task PCB */
extern pcb_t *volatile current_running0;
extern pcb_t *volatile current_running1;
extern pcb_t *volatile current_running;
extern list_head *ready_queue;
extern list_head *ready_queue0;
extern list_head *ready_queue1;
// pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;
extern int cpu_lock;
extern int allocpid;

extern list_node_t send_queue;
extern list_node_t recv_queue;

extern pcb_t pcb[NUM_MAX_TASK];
// pcb_t kernel_pcb[NR_CPUS];
extern const ptr_t pid0_stack;
extern const ptr_t pid0_stack2;
extern pcb_t pid0_pcb;
extern pcb_t pid0_pcb2;

void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);
void check_sleeping();

void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *, int type);

pid_t do_spawn(task_info_t *task, void *arg, spawn_mode_t mode);
void do_exit(void);
int do_kill(pid_t pid);
int do_waitpid(pid_t pid);
void do_process_show();
pid_t do_getpid();
pid_t do_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode);
void do_show_exec();
