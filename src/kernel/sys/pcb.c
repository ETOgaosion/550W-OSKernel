#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/stack.h>
#include <common/elf.h>
#include <drivers/screen.h>
#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <os/time.h>
#include <user/user_programs.h>

pid_t process_id = 1;
int allocpid = 0;

list_node_t *avalable0[20];
list_node_t *avalable1[20];

pcb_t *volatile current_running0;
pcb_t *volatile current_running1;
pcb_t **volatile current_running;

LIST_HEAD(ready_queue);

pcb_t pcb[NUM_MAX_TASK];

const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE * 3 - 112 - 288;
const ptr_t pid0_stack2 = INIT_KERNEL_STACK + PAGE_SIZE * 4 - 112 - 288;
pcb_t pid0_pcb = {.pid = 0, .kernel_sp = (ptr_t)pid0_stack, .user_sp = (ptr_t)pid0_stack, .preempt_count = 0};
pcb_t pid0_pcb2 = {.pid = 0, .kernel_sp = (ptr_t)pid0_stack2, .user_sp = (ptr_t)pid0_stack2, .preempt_count = 0};

int freepidnum = 0;
int nowpid = 1;
int glmask = 3;
int glvalid = 0;

pid_t freepid[NUM_MAX_TASK];

pid_t nextpid() {
    if (freepidnum != 0)
        return freepid[freepidnum--];
    else
        return nowpid++;
}

unsigned x = 0;

extern int check_wait_recv(int num_packet);

void mysrand(unsigned seed) { x = seed; }

int myrand() {
    long long tmp = 0x5deece66dll * x + 0xbll;
    x = tmp & 0x7fffffff;
    return x;
}

void init_pcb() {
    allocpid = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].status = TASK_EXITED;
        pcb[i].stime = 0;
        pcb[i].stime_last = 0;
        pcb[i].utime = 0;
        pcb[i].utime_last = 0;
        memset(&pcb[i].timer, 0, sizeof(pcb[i].timer));
    }
    pcb[0].type = USER_PROCESS;
    pcb[0].pid = 0;
    pcb[0].fpid = 0;
    pcb[0].tid = 0;
    pcb[0].threadsum = 0;
    pcb[0].status = TASK_READY;
    pcb[0].cursor_x = 1;
    pcb[0].cursor_y = 1;
    pcb[0].priority.priority = 0;
    pcb[0].priority.last_sched_time = 0;
    pcb[0].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[0].wait_list.next = &pcb[0].wait_list;
    pcb[0].wait_list.prev = &pcb[0].wait_list;
    pcb[0].core_mask = 3;
    pcb[0].locksum = 0;
    pcb[0].child_num = 0;
    pcb[0].father_pid = 0;
    ptr_t kernel_stack = get_kernel_address(0);
    ptr_t user_stack = kernel_stack - 0x2000;
    allocpid = 0;
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((uintptr_t)pgdir);
    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file("shell", &binary, &len);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_shell = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack), pgdir);
    init_pcb_stack(kernel_stack, USER_STACK_ADDR, (ptr_t)test_shell, &pcb[0], 0, NULL);
    pcb[0].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    current_running0 = &pid0_pcb;
    list_add_tail(&(pcb[0].list), &ready_queue);
    current_running1 = &pid0_pcb2;
    timers.next = &timers;
    timers.prev = &timers;
}

long sys_setpriority(int which, int who, int niceval) {
    int error = -EINVAL;
    if (which > PRIO_USER || which < PRIO_PROCESS) {
        goto sys_setpriority_out;
    }
    switch (which) {
    case PRIO_PROCESS:
        (*current_running)->priority.priority = niceval;
        break;
    case PRIO_PGRP:
        (*current_running)->priority.priority = niceval;
        for (int i = 0; i < (*current_running)->child_num; i++) {
            pcb[(*current_running)->child_pid[i]].priority.priority = niceval;
        }
        for (int i = 0; i < (*current_running)->threadsum; i++) {
            pcb[(*current_running)->threadid[i]].priority.priority = niceval;
        }
        break;
    case PRIO_USER:
        break;
    default:
        break;
    }
sys_setpriority_out:
    return error;
}

long sys_getpriority(int which, int who) {
    return (*current_running)->priority.priority;
}

uint64_t cal_priority(uint64_t cur_time, uint64_t idle_time, long priority) {
    uint64_t mid_div = cur_time / 100, mul_res = 1;
    while (mid_div > 10) {
        mid_div /= 10;
        mul_res *= 10;
    }
    uint64_t cal_res = cur_time - idle_time + priority * mul_res;
    return cal_res;
}

pcb_t *choose_sched_task(list_head *queue) {
    uint64_t cur_time = get_ticks();
    uint64_t max_priority = 0;
    uint64_t cur_priority = 0;
    list_head *list_iterator = queue->next;
    pcb_t *max_one = NULL;
    pcb_t *pcb_iterator = NULL;
    int cpu_id = get_current_cpu_id();
    while (list_iterator->next != queue) {
        pcb_iterator = list_entry(list_iterator, pcb_t, list);
        if (!(pcb_iterator->core_mask & (0x1 << cpu_id))) {
            continue;
        }
        cur_priority = cal_priority(cur_time, pcb_iterator->priority.last_sched_time, pcb_iterator->priority.priority);
        if (max_priority < cur_priority) {
            max_priority = cur_priority;
            max_one = pcb_iterator;
        }
        list_iterator = list_iterator->next;
    }
    max_one->priority.last_sched_time = cur_time;
    return max_one;
}

void enqueue(list_head *new, list_head *head, int target) {
    switch (target) {
    case ENQUEUE_LIST:
        list_add_tail(new, head);
        break;
    case ENQUEUE_PCB:
    {
        pcb_t *curr = list_entry(new, pcb_t, list);
        list_head *insert_point = head->next;
        pcb_t *iterator = list_entry(insert_point, pcb_t, list);
        // use >= because of FIFO
        while (iterator->priority.priority >= curr->priority.priority && insert_point != &ready_queue) {
            insert_point = insert_point->next;
            iterator = list_entry(insert_point, pcb_t, list);
        }
        // for head add to tail, for common position add to front
        list_add_tail(&(curr->list), insert_point);
        break;
    }
    default:
        break;
    }
}

pcb_t *dequeue(list_head *queue, int target) {
    // plain and simple way
    pcb_t *ret = NULL;
    switch (target) {
    case DEQUEUE_LIST:
        ret = list_entry(queue->next, pcb_t, list);
        list_del(&(ret->list));
        break;
    case DEQUEUE_WAITLIST:
        ret = list_entry(queue->next, pcb_t, wait_list);
        list_del(&(ret->wait_list));
        break;
    case DEQUEUE_WAITLIST_DESTROY:
        ret = list_entry(queue->next, pcb_t, wait_list);
        list_del(&(ret->wait_list));
        memset(&ret->timer, 0, sizeof(ret->timer));
        list_init_with_null(&ret->wait_list);
        break;
    case DEQUEUE_LIST_STRATEGY:
        ret = choose_sched_task(queue);
        list_del(&(ret->list));
        break;
    default:
        break;
    }
    return ret;
}

bool check_empty(int cpu_mask) {
    if (list_is_empty(&ready_queue)) {
        return TRUE;
    } else {
        list_head *node = ready_queue.next;
        pcb_t *iterator = NULL;
        while (node != &ready_queue) {
            iterator = list_entry(node, pcb_t, list);
            if (iterator->core_mask & cpu_mask) {
                return FALSE;
            }
            node = node->next;
        }
        return TRUE;
    }
}

long k_scheduler(void) {
    if (!list_is_empty(&timers)) {
        check_sleeping();
    }
    pcb_t *curr = (*current_running);
    int cpuid = get_current_cpu_id();
    bool ready_queue_empty = check_empty(0x1 << cpuid);
    if (ready_queue_empty && curr->pid != 0 && curr->status == TASK_RUNNING) {
        return 0;
    } else if (ready_queue_empty) {
        while (TRUE) {
            unlock_kernel();
            while (check_empty(0x1 << cpuid)) {
                continue;
            }
            lock_kernel();
            if (!check_empty(0x1 << cpuid)) {
                break;
            }
        }
        (*current_running) = curr;
    }
    if (curr->status == TASK_RUNNING && curr->pid != 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_PCB);
    }
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_STRATEGY);
    pcb_move_cursor(screen_cursor_x, screen_cursor_y);
    load_curpcb_cursor();
    next_pcb->status = TASK_RUNNING;
    if (cpuid == 0) {
        current_running0 = next_pcb;
        *current_running = current_running0;
    } else {
        current_running1 = next_pcb;
        *current_running = current_running1;
    }
    set_satp(SATP_MODE_SV39, (*current_running)->pid, (uint64_t)(kva2pa((*current_running)->pgdir)) >> 12);
    local_flush_tlb_all();
    switch_to(curr, next_pcb);
    return 0;
}

long sys_sched_yield(void) {
    return k_scheduler();
}

long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp) {
    (*current_running)->timer.initialized = true;
    copy_nanotime(&(*current_running)->timer.time, rqtp);
    get_nanotime(&(*current_running)->timer.start_time);
    k_block(&((*current_running)->list), &timers);
    k_scheduler();
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the (*current_running)
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the (*current_running) is blocked.
    return 0;
}

void check_sleeping() {
    list_node_t *q;
    nanotime_val_t now_time;
    list_head *p = timers.next;
    while (p != &timers) {
        get_nanotime(&now_time);
        nanotime_val_t res_time;
        pcb_t *p_pcb = list_entry(p, pcb_t, list);
        minus_nanotime(&now_time, &p_pcb->timer.start_time, &res_time);
        if (cmp_nanotime(&res_time, &p_pcb->timer.time) >= 0) {
            q = p->next;
            k_unblock(p, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
            p = q;
        } else
            p = p->next;
    }
}

void k_block(list_node_t *pcb_node, list_head *queue) {
    list_add_tail(pcb_node, queue);
    (*current_running)->status = TASK_BLOCKED;
}

void k_unblock(list_head *from_queue, list_head *to_queue, int way) {
    // TODO: unblock the `pcb` from the block queue
    pcb_t *fetch_pcb = NULL;
    switch (way) {
    case UNBLOCK_TO_LIST_FRONT:
        fetch_pcb = dequeue(from_queue->prev, 0);
        list_add(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_TO_LIST_BACK:
        fetch_pcb = dequeue(from_queue->prev, 0);
        list_add_tail(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_ONLY:
        fetch_pcb = dequeue(from_queue->prev, 0);
        break;
    case UNBLOCK_TO_LIST_STRATEGY:
        fetch_pcb = dequeue(from_queue->prev, 2);
        list_add_tail(&fetch_pcb->list, to_queue);
        break;
    default:
        break;
    }
    fetch_pcb->status = TASK_READY;
}

long k_taskset(int pid, int mask) {
    int id = get_current_cpu_id();
    if (pid == 0) {
        if (mask != 3 && mask != 1 + id) {
            glmask = mask;
            glvalid = 1;
            return 1;
        } else
            glvalid = 0;
    }
    pcb_t *target = &pcb[pid];
    if (pid > NUM_MAX_TASK || target->status == TASK_EXITED || target->status == TASK_ZOMBIE)
        return 0;
    target->core_mask = mask;
    return 1;
}

long sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode) {
    int i = nextpid();
    pcb[i].type = info->type;
    pcb[i].pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority.priority = 0;
    pcb[i].priority.last_sched_time = 0;
    pcb[i].mode = mode;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].pgdir = 0;
    if (glvalid)
        pcb[i].core_mask = glmask;
    else
        pcb[i].core_mask = (*current_running)->core_mask;
    pcb[i].locksum = 0;
    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack = kernel_stack - 0x1000;
    /* TODO: use glibc argument passing standards to restructure */
    /* 1, arg is wrong, only for passing compilation */
    init_pcb_stack(kernel_stack, user_stack, (uintptr_t)(info->entry_point), &pcb[i], (uint64_t)arg, NULL);
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_fork(int prior) {

    pid_t fpid = (*current_running)->pid;
    int i = nextpid();

    pcb[fpid].child_pid[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;
    pcb[i].father_pid = fpid;
    pcb[i].tid = 0;
    pcb[i].fpid = i;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    // pcb[i].priority = prior;
    pcb[i].priority.priority = 0;
    pcb[i].priority.last_sched_time = 0;
    pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    // pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].core_mask = glmask;
    else
        pcb[i].core_mask = (*current_running)->core_mask;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;

    allocpid = i;
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((ptr_t)pgdir);

    fork_pcb_stack(kernel_stack_kva, kernel_stack_kva - 0x1000, &pcb[i]);
    fork_pgtable(pgdir, (pa2kva(pcb[fpid].pgdir << 12)));
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);

    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_exit(int error_code) {
    pcb_t *father = &pcb[(*current_running)->father_pid];
    if (father->child_stat_addrs[(*current_running)->pid]) {
        *father->child_stat_addrs[(*current_running)->pid] = error_code;
    }
    return sys_kill((*current_running)->pid);
}

long sys_kill(pid_t pid) {
    pid -= 1;
    if (pid < 1 || pid >= NUM_MAX_TASK) {
        return -1;
    }
    pcb_t *target = &pcb[pid];
    if (target->pid == 0) {
        return -1;
    }

    // realease lock
    for (int i = 0; i < target->locksum; i++) {
        k_mutex_lock_release(target->lockid[i] - 1);
    }
    if (target->mode == ENTER_ZOMBIE_ON_EXIT) {
        // wake up parent
        if (target->father_pid > 0) {
            pcb_t *parent = &pcb[target->father_pid];
            if (!parent->timer.initialized) {
                k_unblock(&parent->list, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
            }
        }
        target->status = TASK_ZOMBIE;
    } else if (target->mode == AUTO_CLEANUP_ON_EXIT) {
        target->pid = 0;
        target->status = TASK_EXITED;
        if(pcb[pid].type!=USER_THREAD)
            getback_page(pid);
    }
    // give up its sons
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].father_pid == pid) {
            pcb[i].father_pid = target->father_pid;
        }
    }
    if (target->list.next)
        list_del(&target->list);
    if ((*current_running)->pid == pid + 1 || (*current_running)->pid == 0 || target->mode == AUTO_CLEANUP_ON_EXIT) {
        k_scheduler();
    }
    return 0;
}

long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru) {
    pcb_t *target = &pcb[pid];
    while (target->status != TASK_EXITED && target->status != TASK_ZOMBIE) {
        k_block(&(*current_running)->list, &target->wait_list);
        k_scheduler();
    }
    if (target->status == TASK_ZOMBIE) {
        freepidnum++;
        freepid[freepidnum] = pid;
        target->status = TASK_EXITED;
    }
    return 1;
}

long sys_process_show() {
    int num = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].status != TASK_EXITED) {
            if (num == 0)
                prints("[PROCESS TABLE]\n");
            prints("[%d] PID : %d, TID : %d", num, pcb[i].fpid,
                   pcb[i].tid); // pcb[i].tid
            switch (pcb[i].status) {
            case TASK_RUNNING:
                sys_screen_write(" STATUS : RUNNING");
                break;
            case TASK_BLOCKED:
                sys_screen_write(" STATUS : BLOCKED");
                break;
            case TASK_ZOMBIE:
                sys_screen_write(" STATUS : ZOMBIE");
                break;
            case TASK_READY:
                sys_screen_write(" STATUS : READY");
                break;
            default:
                break;
            }
            prints(" MASK : 0x%d", pcb[i].core_mask);
            if (pcb[i].status == TASK_RUNNING) {
                sys_screen_write(" on Core ");
                switch (pcb[i].core_mask) {
                case 1:
                    sys_screen_write("0\n");
                    break;
                case 2:
                    sys_screen_write("1\n");
                    break;
                case 3:
                    if (&pcb[i] == current_running0)
                        sys_screen_write("0\n");
                    else if (&pcb[i] == current_running1)
                        sys_screen_write("1\n");
                    break;
                default:
                    break;
                }
            } else
                sys_screen_write("\n");
            num++;
        }
    }
    return num + 1;
}

extern int try_get_from_file(const char *file_name, unsigned char **binary, int *length);
long sys_exec(const char *name, int argc, char **argv) {
    int len = 0;
    unsigned char *binary = NULL;
    /* TODO: use FAT32 disk to read program */
    if (get_elf_file(name, &binary, &len) == 0) {
        if (try_get_from_file(name, &binary, &len) == 0)
            return 0;
    }
    int i = nextpid();
    pcb[i].fpid = i;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].tid = 0;
    pcb[i].threadsum = 0;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority.priority = 0;
    pcb[i].priority.last_sched_time = 0;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    pcb[i].father_pid = i;
    if (glvalid)
        pcb[i].core_mask = glmask;
    else
        pcb[i].core_mask = (*current_running)->core_mask;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    allocpid = i;
    PTE *pgdir = (PTE *)allocPage(1);
    clear_pgdir((uintptr_t)pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    void_task test_task = (void_task)load_elf(binary, len, (uintptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))alloc_page_helper);
    map(USER_STACK_ADDR - 0x1000, kva2pa(user_stack_kva), pgdir);
    init_pcb_stack(kernel_stack_kva, USER_STACK_ADDR, (ptr_t)test_task, &pcb[i], argc, argv);
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_getpid() { return (*current_running)->fpid; }

long sys_getppid() { return (*current_running)->father_pid; }

long sys_mthread_create(pid_t *thread, void (*start_routine)(void *), void *arg) {
    pid_t fpid = (*current_running)->pid;
    int i = nextpid();
    pcb[fpid].threadid[pcb[fpid].threadsum] = i;
    pcb[fpid].threadsum++;
    pcb[i].father_pid = i;
    pcb[i].tid = pcb[fpid].threadsum;
    pcb[i].fpid = fpid;
    pcb[i].type = USER_THREAD;
    pcb[i].pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority.priority = 0;
    pcb[i].priority.last_sched_time = 0;
    // pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].core_mask = glmask;
    else
        pcb[i].core_mask = (*current_running)->core_mask;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    uint64_t user_stack_uva = USER_STACK_ADDR + 0x1000 * pcb[fpid].threadsum + 0x1000;
    pcb[i].pgdir = pcb[fpid].pgdir;
    map(user_stack_uva - 0x1000, kva2pa(user_stack_kva), (pa2kva(pcb[i].pgdir << 12)));
    init_pcb_stack(kernel_stack_kva, (ptr_t)user_stack_uva, (ptr_t)start_routine, &pcb[i], (uint64_t)arg, NULL);
    // add pcb to the ready
    list_add_tail(&pcb[i].list, &ready_queue);
    *thread = i;
    return 1;
}