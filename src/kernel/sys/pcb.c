#include <asm/atomic.h>
#include <asm/pgtable.h>
#include <asm/stack.h>
#include <common/elf.h>
#include <drivers/screen/screen.h>
#include <fs/fs.h>
#include <lib/math.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/irq.h>
#include <os/pcb.h>
#include <os/smp.h>
#include <user/user_programs.h>

pcb_t *volatile current_running0;
pcb_t *volatile current_running1;
pcb_t *volatile *volatile current_running;

LIST_HEAD(ready_queue);
LIST_HEAD(block_queue);

pcb_t pcb[NUM_MAX_TASK];

const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE * 3 - 112 - 288;
const ptr_t pid0_stack2 = INIT_KERNEL_STACK + PAGE_SIZE * 5 - 112 - 288;
pcb_t pid0_pcb = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack, .user_sp = (ptr_t)(INIT_KERNEL_STACK + PAGE_SIZE * 2), .core_mask[0] = 0x3, .status = TASK_EXITED};
pcb_t pid0_pcb2 = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack2, .user_sp = (ptr_t)(INIT_KERNEL_STACK + PAGE_SIZE * 4), .core_mask[0] = 0x3, .status = TASK_EXITED};

pid_t freepid[NUM_MAX_TASK];

pid_t nextpid() {
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (!pcb[i].in_use) {
            return i;
        }
    }
    while (true) {
        return nextpid();
    }
}

void init_pcb_i(char *name, int pcb_i, task_type_t type, int pid, int fpid, int tid, int father_pid, uint8_t core_mask) {
    k_memset((uint8_t *)pcb[pcb_i].name, 0, sizeof(pcb[pcb_i].name));
    k_memcpy((uint8_t *)pcb[pcb_i].name, (uint8_t *)name, k_strlen(name));
    pcb[pcb_i].in_use = TRUE;
    pcb[pcb_i].pid = pid;
    pcb[pcb_i].fpid = fpid;
    pcb[pcb_i].tid = tid;
    pcb[pcb_i].father_pid = father_pid;
    pcb[pcb_i].child_num = 0;
    k_memset((void *)pcb[pcb_i].child_pids, 0, sizeof(pcb[pcb_i].child_pids));
    k_memset((void *)pcb[pcb_i].child_stat_addrs, 0, sizeof(pcb[pcb_i].child_stat_addrs));
    pcb[pcb_i].threadsum = 0;
    k_memset((void *)pcb[pcb_i].thread_ids, 0, sizeof(pcb[pcb_i].child_pids));
    pcb[pcb_i].type = type;
    pcb[pcb_i].status = TASK_READY;
    pcb[pcb_i].cursor_x = 1;
    pcb[pcb_i].cursor_y = 1;
    pcb[pcb_i].priority.priority = 1;
    pcb[pcb_i].priority.last_sched_time = 0;
    pcb[pcb_i].core_mask[0] = core_mask;
    pcb[pcb_i].locksum = 0;
    pcb[pcb_i].mbox = &pcb_mbox[pcb_i];
    k_pcb_mbox_init(pcb[pcb_i].mbox, pcb_i);
    k_memset((void *)&(pcb[pcb_i].stime_last), 0, sizeof(__kernel_time_t));
    k_memset((void *)&(pcb[pcb_i].utime_last), 0, sizeof(__kernel_time_t));
    k_memset((void *)&(pcb[pcb_i].timer), 0, sizeof(pcbtimer_t));
    pcb[pcb_i].dead_child_stime = 0;
    pcb[pcb_i].dead_child_utime = 0;
    k_memset((void *)&(pcb[pcb_i].resources), 0, sizeof(rusage_t));
}

uintptr_t init_user_stack(ptr_t *user_stack_kva, ptr_t *user_stack, int argc, const char *argv[], int envpc, const char *envp[], const char *file_name) {
    int total_length = (argc + envpc + 3) * sizeof(uintptr_t *);
    int pointers_length = total_length;
    for (int i = 0; i < argc; i++) {
        total_length += (k_strlen(argv[i]) + 1);
    }
    for (int i = 0; i < envpc; i++) {
        total_length += (k_strlen(envp[i]) + 1);
    }
    total_length += (k_strlen(file_name) + 1);
    total_length = K_ROUND(total_length, 0x100);
    uintptr_t kargv_pointer = (uintptr_t)*user_stack_kva - total_length;
    intptr_t kargv = kargv_pointer + pointers_length;

    /* 1. store argc in the lowest */
    *((int *)kargv_pointer) = argc;
    kargv_pointer += sizeof(uint64_t);

    /* 2. save argv pointer in 2nd lowest and argv in lowest strings */
    uintptr_t new_argv = (uintptr_t)*user_stack - total_length + pointers_length;
    uintptr_t ret = new_argv;
    for (int i = 0; i < argc; i++) {
        *((uintptr_t *)kargv_pointer + i) = new_argv;
        k_strcpy((char *)kargv, argv[i]);
        new_argv += (k_strlen(argv[i]) + 1);
        kargv += (k_strlen(argv[i]) + 1);
    }
    *((uintptr_t *)kargv_pointer + argc) = 0;
    kargv_pointer += (argc + 1) * sizeof(uintptr_t);

    /* 3. save envp pointer in 3rd lowest and envp in 2nd lowest strings */
    for (int i = 0; i < envpc; i++) {
        *((uintptr_t *)kargv_pointer + i) = new_argv;
        k_strcpy((char *)kargv, envp[i]);
        new_argv += k_strlen(envp[i]) + 1;
        kargv += k_strlen(envp[i]) + 1;
    }
    *((uintptr_t *)kargv_pointer + envpc) = 0;

    /* 4. save file_name */
    k_strcpy((char *)kargv, file_name);
    new_argv = new_argv + k_strlen(file_name) + 1;
    kargv += k_strlen(file_name) + 1;

    /* now user_sp is user_stack - total_length */
    *user_stack_kva -= total_length;
    *user_stack -= total_length;
    return ret;
}

void k_pcb_init() {
    k_memset(&pcb, 0, sizeof(pcb));
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].pid = -1;
        pcb[i].status = TASK_EXITED;
    }
    current_running0 = &pid0_pcb;
    current_running1 = &pid0_pcb2;
    current_running = k_smp_get_current_running();
    init_list_head(&timers);
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
        for (int i = 0; i < NUM_MAX_CHILD; i++) {
            if ((*current_running)->child_pids[i] != 0) {
                pcb[(*current_running)->child_pids[i]].priority.priority = niceval;
            }
        }
        for (int i = 0; i < (*current_running)->threadsum; i++) {
            pcb[(*current_running)->thread_ids[i]].priority.priority = niceval;
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

pcb_t *check_first_ready_task() {
    list_head *iterator = ready_queue.next;
    pcb_t *pcb_it = NULL;
    while (iterator != &ready_queue) {
        pcb_it = list_entry(iterator, pcb_t, list);
        if (k_strcmp(pcb_it->name, "bubble")) {
            return pcb_it;
        }
        iterator = iterator->next;
    }
    return NULL;
}

pcb_t *choose_sched_task(list_head *queue) {
    uint64_t cur_time = k_time_get_ticks();
    uint64_t max_priority = 0;
    uint64_t cur_priority = 0;
    list_head *list_iterator = queue->next;
    pcb_t *max_one = NULL;
    pcb_t *pcb_iterator = NULL;
    int cpu_id = k_smp_get_current_cpu_id();
    while (list_iterator->next != queue) {
        pcb_iterator = list_entry(list_iterator, pcb_t, list);
        if (!(pcb_iterator->core_mask[cpu_id / 8] & (0x1 << (cpu_id % 8)))) {
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

void enqueue(list_head *new, list_head *head, enqueue_way_t way) {
    switch (way) {
    case ENQUEUE_LIST:
        list_add_tail(new, head);
        break;
    case ENQUEUE_TIMER_LIST: {
        pcb_t *new_inserter = list_entry(new, pcb_t, list);
        list_head *iterator_list = timers.next;
        pcb_t *iterator_pcb = NULL;
        while (iterator_list != &timers) {
            iterator_pcb = list_entry(iterator_list, pcb_t, list);
            if (k_time_cmp_nanotime(&iterator_pcb->timer.end_time, &new_inserter->timer.end_time) >= 0) {
                break;
            }
        }
        list_add_tail(new, iterator_list);
        break;
    }
    default:
        break;
    }
}

pcb_t *dequeue(list_head *queue, dequeue_way_t target) {
    // plain and simple way
    pcb_t *ret = NULL;
    pcb_t *task = check_first_ready_task();
    switch (target) {
    case DEQUEUE_LIST:
        ret = list_entry(queue->next, pcb_t, list);
        list_del(&(ret->list));
        break;
    case DEQUEUE_LIST_STRATEGY:
        // ret = choose_sched_task(queue);
        if (!task) {
            ret = list_entry(queue->next, pcb_t, list);
        }
        else {
            ret = task;
        }
        list_del(&(ret->list));
        break;
    default:
        break;
    }
    return ret;
}

long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp) {
    (*current_running)->timer.initialized = TRUE;
    k_time_get_nanotime(&(*current_running)->timer.start_time);
    k_time_add_nanotime(rqtp, &(*current_running)->timer.start_time, &(*current_running)->timer.end_time);
    (*current_running)->timer.remain_time = rmtp;
    k_pcb_block(&((*current_running)->list), &timers, ENQUEUE_TIMER_LIST);
    k_pcb_scheduler();
    return 0;
}

void check_sleeping() {
    list_node_t *q;
    nanotime_val_t now_time;
    list_head *p = timers.next;
    k_time_get_nanotime(&now_time);
    while (p != &timers) {
        pcb_t *p_pcb = list_entry(p, pcb_t, list);
        if (k_time_cmp_nanotime(&now_time, &p_pcb->timer.end_time) >= 0) {
            q = p->next;
            k_pcb_unblock(p, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
            p = q;
        } else {
            break;
        }
    }
}

long k_pcb_scheduler(void) {
    if (!list_is_empty(&timers)) {
        check_sleeping();
    }
    pcb_t *curr = (*current_running);
    int cpuid = k_smp_get_current_cpu_id();
    if (curr->status == TASK_RUNNING && curr->pid >= 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_LIST);
    }
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_STRATEGY);
    // d_screen_pcb_move_cursor(screen_cursor_x, screen_cursor_y);
    // d_screen_load_curpcb_cursor();
    next_pcb->status = TASK_RUNNING;
    if (cpuid == 0) {
        current_running0 = next_pcb;
        current_running = &current_running0;
    } else {
        current_running1 = next_pcb;
        current_running = &current_running1;
    }
    set_satp(SATP_MODE_SV39, (*current_running)->pid + 1, (*current_running)->pgdir);
    local_flush_tlb_all();
    switch_to(curr, next_pcb);
    return 0;
}

long sys_sched_yield(void) {
    return k_pcb_scheduler();
}

void k_pcb_block(list_node_t *pcb_node, list_head *queue, enqueue_way_t way) {
    enqueue(pcb_node, queue, way);
    (*current_running)->status = TASK_BLOCKED;
}

void k_pcb_unblock(list_head *from_queue, list_head *to_queue, unblock_way_t way) {
    // TODO: unblock the `pcb` from the block queue
    pcb_t *fetch_pcb = NULL;
    switch (way) {
    case UNBLOCK_TO_LIST_FRONT:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST);
        list_add(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_TO_LIST_BACK:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST);
        list_add_tail(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_ONLY:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST);
        break;
    case UNBLOCK_TO_LIST_STRATEGY:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST_STRATEGY);
        list_add_tail(&fetch_pcb->list, to_queue);
        break;
    default:
        break;
    }
    fetch_pcb->status = TASK_READY;
}

long sys_sched_setaffinity(pid_t pid, unsigned int len, const byte_size_t *user_mask_ptr) {
    if (pid > NUM_MAX_TASK) {
        return -EINVAL;
    }
    pcb_t *target = pid ? &pcb[pid] : (*current_running);
    if (target->status == TASK_EXITED) {
        return -EINVAL;
    }
    int core_len = CPU_SET_SIZE;
    int copy_len = k_min(core_len, len);
    k_memcpy(target->core_mask, user_mask_ptr, copy_len);
    return 0;
}

long sys_sched_getaffinity(pid_t pid, unsigned int len, uint8_t *user_mask_ptr) {
    if (pid > NUM_MAX_TASK) {
        return -EINVAL;
    }
    pcb_t *target = pid ? &pcb[pid] : (*current_running);
    if (target->status == TASK_EXITED) {
        return -EINVAL;
    }
    int core_len = CPU_SET_SIZE;
    int copy_len = k_min(core_len, len);
    k_memcpy(user_mask_ptr, target->core_mask, copy_len);
    return 0;
}

long spawn(const char *file_name) {
    int i = nextpid();

    init_pcb_i((char *)file_name, i, USER_PROCESS, i, i, 0, (*current_running)->father_pid, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack - 4 * PAGE_SIZE;
    ptr_t user_stack = USER_STACK_ADDR;
    PTE *pgdir = (PTE *)k_mm_alloc_page(1);
    clear_pgdir((uintptr_t)pgdir);
    k_mm_share_pgtable(pgdir, pa2kva(PGDIR_PA));
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file(file_name, &binary, &len);
    void_task process = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))k_mm_alloc_page_helper);
    k_mm_map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    // k_mm_map(USER_STACK_ADDR, kva2pa(user_stack_kva), pgdir);
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    init_context_stack(kernel_stack, user_stack, 0, NULL, (uintptr_t)(process), &pcb[i]);
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_spawn(const char *file_name) {
    // check file existence
    if (!fs_check_file_existence(file_name)) {
        return -EINVAL;
    }
    return spawn(file_name);
}

long sys_fork() {
    int i = nextpid();
    pid_t fpid = (*current_running)->pid;

    char name[NUM_MAX_PCB_NAME];
    int name_len = k_min(k_strlen((*current_running)->name), 14);
    k_memcpy((uint8_t *)name, (uint8_t *)(*current_running)->name, name_len);
    k_strcat(name, "_child");
    init_pcb_i(name, i, USER_PROCESS, i, i, 0, fpid, (*current_running)->core_mask[0]);

    pcb[fpid].child_pids[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 4 * PAGE_SIZE;
    PTE *pgdir = (PTE *)k_mm_alloc_page(1);
    clear_pgdir((ptr_t)pgdir);
    k_mm_share_pgtable(pgdir, pa2kva(PGDIR_PA));

    fork_pcb_stack(kernel_stack_kva, user_stack_kva, &pcb[i]);
    k_mm_fork_pgtable(pgdir, (pa2kva((*current_running)->pgdir << NORMAL_PAGE_SHIFT)));
    k_mm_map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    // k_mm_map(USER_STACK_ADDR, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);

    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

int kill(pid_t pid, int exit_status) {
    pcb_t *target = &pcb[pid];
    // realease lock
    for (int i = 0; i < target->locksum; i++) {
        k_mutex_lock_release(target->lock_ids[i] - 1);
    }
    if (target->father_pid >= 0) {
        pcb_t *parent = &pcb[target->father_pid];
        if (!parent->timer.initialized && parent->status == TASK_BLOCKED) {
            k_pcb_unblock(&parent->list, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        }
        parent->dead_child_stime += k_time_get_ticks_from_time(&target->resources.ru_stime);
        parent->dead_child_utime += k_time_get_ticks_from_time(&target->resources.ru_utime);
        if (parent->cursor_y < target->cursor_y) {
            parent->cursor_y = target->cursor_y;
        }
    }
    // k_pcb_unblock(&(target->wait_list), &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
    target->pid = 0;
    target->status = TASK_EXITED;
    if (pcb[pid].type != USER_THREAD) {
        k_mm_getback_page(pid);
    }
    // give up its sons
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].father_pid == pid) {
            pcb[i].father_pid = target->father_pid;
        }
    }
    if (target->list.next) {
        list_del(&target->list);
    }
    target->exit_status = exit_status;
    target->pid = -1;
    return 0;
}

long sys_kill(pid_t pid) {
    if (pid <= 0 || pid >= NUM_MAX_TASK) {
        return -EINVAL;
    }
    kill(pid, -1);
    pcb[pid].in_use = FALSE;
    if (pid == (*current_running)->pid) {
        k_pcb_scheduler();
    }
    return 0;
}

long sys_exit(int error_code) {
    int id = k_smp_get_current_cpu_id();
    
    kill((*current_running)->pid, error_code);

    if (id == 0) {
        current_running0 = &pid0_pcb;
    } else if (id == 1) {
        current_running1 = &pid0_pcb2;
    }
    current_running = k_smp_get_current_running();
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    k_pcb_scheduler();
    return 0;
}

long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru) {
    pcb_t *target = NULL;
    int wait_pid = -1, pid_i = -1;
    if (pid != -1) {
        target = &pcb[pid];
        wait_pid = pid;
    } else {
        for (int i = 0; i < NUM_MAX_CHILD; i++) {
            if ((*current_running)->child_pids[i] != 0) {
                pid_i = i;
                wait_pid = (*current_running)->child_pids[i];
                target = &pcb[wait_pid];
                break;
            }
        }
    }
    if (!target->in_use) {
        return wait_pid;
    }
    // no child process, but wait
    if (wait_pid == -1) {
        return 0;
    }
    while (target->status != TASK_EXITED) {
        k_pcb_block(&(*current_running)->list, &block_queue, ENQUEUE_LIST);
        k_pcb_scheduler();
    }
    target = &pcb[wait_pid];
    if (target->in_use) {
        target->status = TASK_EXITED;
        target->in_use = FALSE;
        (*current_running)->child_pids[pid_i] = 0;
        if (stat_addr) {
            *stat_addr = (target->exit_status << 8);
        }
        if (ru) {
            k_memcpy((uint8_t *)ru, (uint8_t *)&target->resources, sizeof(rusage_t));
        }
    }
    return wait_pid;
}

long sys_process_show() {
    int num = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].status != TASK_EXITED) {
            if (num == 0) {
                prints("\n[PROCESS TABLE]\n");
            }
            prints("[%d] PID : %d, TID : %d", num, pcb[i].fpid,
                   pcb[i].tid); // pcb[i].tid
            switch (pcb[i].status) {
            case TASK_RUNNING:
                sys_screen_write(" STATUS : RUNNING");
                break;
            case TASK_BLOCKED:
                sys_screen_write(" STATUS : BLOCKED");
                break;
            case TASK_READY:
                sys_screen_write(" STATUS : READY");
                break;
            case TASK_EXITED:
                sys_screen_write(" STATUS : EXITED");
                break;
            default:
                break;
            }
            prints(" MASK : 0x%x", (int)pcb[i].core_mask[0]);
            if (pcb[i].status == TASK_RUNNING) {
                sys_screen_write(" on Core ");
                switch (pcb[i].core_mask[0]) {
                case 1:
                    sys_screen_write("0");
                    break;
                case 2:
                    sys_screen_write("1");
                    break;
                case 3:
                    if (&pcb[i] == current_running0) {
                        sys_screen_write("0");
                    } else if (&pcb[i] == current_running1) {
                        sys_screen_write("1");
                    }
                    break;
                default:
                    break;
                }
            } else {
                // sys_screen_write("\n");
            }
            num++;
        }
    }
    return num;
}

long exec(int target_pid, int father_pid, const char *file_name, const char *argv[], const char *envp[]) {
    init_pcb_i((char *)file_name, target_pid, USER_PROCESS, target_pid, target_pid, 0, father_pid, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(target_pid);
    ptr_t user_stack_kva = kernel_stack - 4 * PAGE_SIZE;
    ptr_t user_stack = USER_STACK_ADDR;
    // k_mm_getback_page(target_pid);
    PTE *pgdir = (PTE *)k_mm_alloc_page(1);
    clear_pgdir((uintptr_t)pgdir);
    k_mm_share_pgtable(pgdir, pa2kva(PGDIR_PA));
    pcb[target_pid].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file(file_name, &binary, &len);
    void_task process = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))k_mm_alloc_page_helper);
    k_mm_map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    // k_mm_map(USER_STACK_ADDR, kva2pa(user_stack_kva), pgdir);
    pcb[target_pid].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    int argc = k_strlistlen((char **)argv);
    int envpc = k_strlistlen((char **)envp);
    uintptr_t child_argv = init_user_stack(&user_stack_kva, &user_stack, argc, argv, envpc, envp, file_name);

    init_context_stack(kernel_stack, user_stack, argc, (char **)child_argv, (ptr_t)process, &pcb[target_pid]);

    list_add_tail(&pcb[target_pid].list, &ready_queue);
    return target_pid;
}

long sys_exec(const char *file_name, const char *argv[], const char *envp[]) {
    return exec(nextpid(), (*current_running)->pid, file_name, argv, envp);
}

long sys_execve(const char *file_name, const char *argv[], const char *envp[]) {
    int ret;
    ret = exec((*current_running)->pid, (*current_running)->father_pid, file_name, argv, envp);
    if (ret) {
        k_pcb_scheduler();
    }
    return ret;
}

long clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    int i = nextpid();
    pid_t fpid = (*current_running)->pid;

    char name[NUM_MAX_PCB_NAME] = {0};
    int name_len = k_min(k_strlen((*current_running)->name), 14);
    k_memcpy((uint8_t *)name, (uint8_t *)(*current_running)->name, name_len);
    k_strcat(name, "_child");
    init_pcb_i(name, i, USER_PROCESS, i, i, 0, fpid, (*current_running)->core_mask[0]);

    if (flags & CLONE_CHILD_SETTID) {
        *(int *)child_tid = pcb[i].tid;
    }
    if (flags & CLONE_PARENT_SETTID) {
        *(int *)parent_tid = pcb[i].tid;
    }
    if (flags & CLONE_CHILD_CLEARTID) {
        pcb[i].clear_ctid = (uint32_t *)child_tid;
    }

    pcb[fpid].child_pids[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 4 * PAGE_SIZE;

    if (flags & CLONE_VM) {
        pcb[i].pgdir = (*current_running)->pgdir;

    } else {
        PTE *pgdir = (PTE *)k_mm_alloc_page(1);
        clear_pgdir((ptr_t)pgdir);

        k_mm_fork_pgtable(pgdir, (pa2kva((*current_running)->pgdir << NORMAL_PAGE_SHIFT)));
        k_mm_share_pgtable(pgdir, pa2kva(PGDIR_PA));
        pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    }

    k_memcpy((uint8_t *)&pcb[i].elf, (uint8_t *)&(*current_running)->elf, sizeof(ELF_info_t));
    k_memcpy((uint8_t *)(user_stack_kva - NORMAL_PAGE_SIZE), (const uint8_t *)(K_ROUNDDOWN((*current_running)->user_sp, NORMAL_PAGE_SIZE)), NORMAL_PAGE_SIZE);

    ptr_t user_stack = (ptr_t)stack ? (ptr_t)stack : USER_STACK_ADDR;
    // ptr_t user_stack = USER_STACK_ADDR;
    clone_pcb_stack(kernel_stack_kva, user_stack, &pcb[i], flags, tls);
    if (!stack) {
        k_mm_map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pa2kva(pcb[i].pgdir << NORMAL_PAGE_SHIFT));
    }
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    return clone(flags, stack, parent_tid, tls, child_tid); 
}

long k_pcb_getpid(void) {
    return (*current_running)->fpid;
}

/**
 * @brief [SYSCALL] getpid: get current task process id
 * 
 * @return long 
 */
long sys_getpid() {
    return k_pcb_getpid();
}

/**
 * @brief [SYSCALL] getppid: get parent of current task pid
 * 
 * @return long 
 */
long sys_getppid() {
    return (*current_running)->father_pid + 1;
}

void k_pcb_sleep(void *chan, spin_lock_t *lk) {
    k_spin_lock_release(lk);
    (*current_running)->chan = chan;
    k_pcb_block(&(*current_running)->list, &block_queue, ENQUEUE_LIST);
    k_pcb_scheduler();
    (*current_running)->chan = 0;
    k_spin_lock_acquire(lk);
}

void k_pcb_wakeup(void *chan) {
    for (int i = 0; i < NUM_MAX_PROCESS; i++) {
        if ((pcb[i].status == TASK_BLOCKED) && (pcb[i].chan == chan)) {
            k_pcb_unblock(&pcb[i].list, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        }
    }
}