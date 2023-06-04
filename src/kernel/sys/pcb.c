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
const ptr_t pid0_stack2 = INIT_KERNEL_STACK + PAGE_SIZE * 4 - 112 - 288;
pcb_t pid0_pcb = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack, .user_sp = (ptr_t)pid0_stack, .core_mask[0] = 0x3, .status = TASK_EXITED};
pcb_t pid0_pcb2 = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack2, .user_sp = (ptr_t)pid0_stack2, .core_mask[0] = 0x3, .status = TASK_EXITED};

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
    init_list_head(&pcb[pcb_i].wait_list);
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

void init_user_stack(ptr_t *user_stack_kva, ptr_t *user_stack, const char *argv[], const char *envp[], const char *file_name) {
    int argc = k_strlistlen((char **)argv);
    int envpc = k_strlistlen((char **)envp);
    int total_length = (argc + envpc + 3) * sizeof(uintptr_t *);
    int pointers_length = total_length;
    for (int i = 0; i < argc; i++) {
        total_length += (k_strlen(argv[i]) + 1);
    }
    for (int i = 0; i < envpc; i++) {
        total_length += (k_strlen(envp[i]) + 1);
    }
    total_length += (k_strlen(file_name) + 1);
    total_length = ROUND(total_length, 0x100);
    uintptr_t kargv_pointer = (uintptr_t)user_stack_kva - total_length;
    intptr_t kargv = kargv_pointer + pointers_length;

    /* 1. store argc in the lowest */
    *((int *)kargv_pointer) = argc;
    kargv_pointer += sizeof(uint64_t);

    /* 2. save argv pointer in 2nd lowest and argv in lowest strings */
    uintptr_t new_argv = (uintptr_t)user_stack - total_length + pointers_length;
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
    user_stack_kva -= total_length;
    user_stack -= total_length;
}

void k_init_pcb() {
    k_memset(&pcb, 0, sizeof(pcb));
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].pid = -1;
        pcb[i].status = TASK_EXITED;
    }
    current_running0 = &pid0_pcb;
    current_running1 = &pid0_pcb2;
    current_running = k_get_current_running();
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
            if((*current_running)->child_pids[i] != 0) {
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
            if (cmp_nanotime(&iterator_pcb->timer.end_time, &new_inserter->timer.end_time) >= 0) {
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
        k_memset(&ret->timer, 0, sizeof(pcbtimer_t));
        list_init_with_null(&ret->wait_list);
        break;
    case DEQUEUE_LIST_STRATEGY:
        // ret = choose_sched_task(queue);
        ret = list_entry(queue->next, pcb_t, list);
        list_del(&(ret->list));
        break;
    default:
        break;
    }
    return ret;
}

bool check_empty(int cpuid) {
    if (list_is_empty(&ready_queue)) {
        return TRUE;
    } else {
        list_head *node = ready_queue.next;
        pcb_t *iterator = NULL;
        while (node != &ready_queue) {
            iterator = list_entry(node, pcb_t, list);
            if (iterator->core_mask[cpuid / 8] & (0x1 << (cpuid % 8))) {
                return false;
            }
            node = node->next;
        }
        return TRUE;
    }
}

long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp) {
    (*current_running)->timer.initialized = true;
    get_nanotime(&(*current_running)->timer.start_time);
    add_nanotime(rqtp, &(*current_running)->timer.start_time, &(*current_running)->timer.end_time);
    (*current_running)->timer.remain_time = rmtp;
    k_block(&((*current_running)->list), &timers, ENQUEUE_TIMER_LIST);
    k_scheduler();
    return 0;
}

void check_sleeping() {
    list_node_t *q;
    nanotime_val_t now_time;
    list_head *p = timers.next;
    get_nanotime(&now_time);
    while (p != &timers) {
        pcb_t *p_pcb = list_entry(p, pcb_t, list);
        if (cmp_nanotime(&now_time, &p_pcb->timer.end_time) >= 0) {
            q = p->next;
            k_unblock(p, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
            p = q;
        } else {
            break;
        }
    }
}

long k_scheduler(void) {
    if (!list_is_empty(&timers)) {
        check_sleeping();
    }
    pcb_t *curr = (*current_running);
    int cpuid = get_current_cpu_id();
    bool ready_queue_empty = check_empty(cpuid);
    if (ready_queue_empty && curr->pid >= 0 && curr->status == TASK_RUNNING) {
        return 0;
    } else if (ready_queue_empty) {
        while (TRUE) {
            k_unlock_kernel();
            while (check_empty(0x1 << cpuid)) {
                continue;
            }
            k_lock_kernel();
            if (!check_empty(0x1 << cpuid)) {
                break;
            }
            k_unlock_kernel();
        }
        (*current_running) = curr;
    }
    if (curr->status == TASK_RUNNING && curr->pid >= 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_LIST);
    }
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_STRATEGY);
    // pcb_move_cursor(screen_cursor_x, screen_cursor_y);
    // load_curpcb_cursor();
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
    return k_scheduler();
}

void k_block(list_node_t *pcb_node, list_head *queue, enqueue_way_t way) {
    enqueue(pcb_node, queue, way);
    (*current_running)->status = TASK_BLOCKED;
}

void k_unblock(list_head *from_queue, list_head *to_queue, unblock_way_t way) {
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

long sys_spawn(const char *file_name) {
    int i = nextpid();

    init_pcb_i((char *)file_name, i, USER_PROCESS, i, i, 0, (*current_running)->father_pid, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack - PAGE_SIZE;
    ptr_t user_stack = USER_STACK_ADDR;
    PTE *pgdir = (PTE *)k_alloc_page(1);
    clear_pgdir((uintptr_t)pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file(file_name, &binary, &len);
    void_task process = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))k_alloc_page_helper);
    map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    map(USER_STACK_ADDR, kva2pa(user_stack_kva), pgdir);
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    init_context_stack(kernel_stack, user_stack, (uintptr_t)(process), &pcb[i]);
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
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
    ptr_t user_stack_kva = kernel_stack_kva - PAGE_SIZE;
    PTE *pgdir = (PTE *)k_alloc_page(1);
    clear_pgdir((ptr_t)pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));

    fork_pcb_stack(kernel_stack_kva, user_stack_kva, &pcb[i]);
    fork_pgtable(pgdir, (pa2kva((*current_running)->pgdir << NORMAL_PAGE_SHIFT)));
    map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    map(USER_STACK_ADDR, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);

    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_kill(pid_t pid) {
    if (pid < 0 || pid >= NUM_MAX_TASK) {
        return -EINVAL;
    }
    pcb_t *target = &pcb[pid];
    if (target->pid == 0) {
        return -EINVAL;
    }

    // realease lock
    for (int i = 0; i < target->locksum; i++) {
        k_mutex_lock_release(target->lock_ids[i] - 1);
    }
    if (target->father_pid >= 0) {
        pcb_t *parent = &pcb[target->father_pid];
        if (!parent->timer.initialized) {
            k_unblock(&parent->list, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        }
        for (int i = 0; i < NUM_MAX_CHILD; i++) {
            if (parent->child_pids[i] == pid) {
                parent->child_pids[i] = 0;
                break;
            }
        }
        parent->dead_child_stime += get_ticks_from_time(&target->resources.ru_stime);
        parent->dead_child_utime += get_ticks_from_time(&target->resources.ru_utime);
    }
    // k_unblock(&(target->wait_list), &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
    target->pid = 0;
    target->status = TASK_EXITED;
    target->in_use = FALSE;
    if (pcb[pid].type != USER_THREAD) {
        getback_page(pid);
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
    target->exit_status = -1;
    return 0;
}

long sys_exit(int error_code) {
    int id = get_current_cpu_id();
    // if ((*current_running)->father_pid >= 0) {
    //     pcb_t *father = &pcb[(*current_running)->father_pid];
        // if (father->child_stat_addrs[(*current_running)->pid]) {
        //     *father->child_stat_addrs[(*current_running)->pid] = error_code;
        // }
    // }
    sys_kill((*current_running)->pid);
    (*current_running)->exit_status = error_code;
    (*current_running)->pid = -1;

    if (id == 0)
        current_running0 = &pid0_pcb;
    else if (id == 1)
        current_running1 = &pid0_pcb2;
    current_running = k_get_current_running();
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    k_scheduler();
    return 0;
}

long sys_wait4(pid_t pid, int *stat_addr, int options, rusage_t *ru) {
    pcb_t *target = NULL;
    int wait_pid = -1;
    if(pid != -1) {
        target = &pcb[pid];
        wait_pid = pid;
    } else {
        for (int i = 0; i < NUM_MAX_CHILD; i++) {
            if ((*current_running)->child_pids[i] != 0) {
                wait_pid = (*current_running)->child_pids[i];
                target = &pcb[wait_pid];
                break;
            }
        }
    }
    // no child process, but wait
    if(wait_pid == -1) {
        return 0;
    }
    while (target->status != TASK_EXITED) {
        k_block(&(*current_running)->list, &target->wait_list, ENQUEUE_LIST);
        k_scheduler();
    }
    target->status = TASK_EXITED;
    if (stat_addr) {
        *stat_addr = (target->exit_status << 8);
    }
    if(ru) {
        k_memcpy((uint8_t *)ru, (uint8_t *)&target->resources, sizeof(rusage_t));
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

long exec(int pid, const char *file_name, const char *argv[], const char *envp[]) {
    int i = nextpid();

    init_pcb_i((char *)file_name, i, USER_PROCESS, i, i, 0, 0, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack - PAGE_SIZE;
    ptr_t user_stack = USER_STACK_ADDR;
    PTE *pgdir = (PTE *)k_alloc_page(1);
    clear_pgdir((uintptr_t)pgdir);
    share_pgtable(pgdir, pa2kva(PGDIR_PA));
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    int len = 0;
    unsigned char *binary = NULL;
    get_elf_file(file_name, &binary, &len);
    void_task process = (void_task)load_elf(binary, len, (ptr_t)pgdir, (uintptr_t(*)(uintptr_t, uintptr_t))k_alloc_page_helper);
    map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pgdir);
    map(USER_STACK_ADDR, kva2pa(user_stack_kva), pgdir);
    pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;

    init_user_stack(&user_stack_kva, &user_stack, argv, envp, file_name);

    init_context_stack(kernel_stack, user_stack, (ptr_t)process, &pcb[i]);

    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long sys_exec(const char *file_name, const char *argv[], const char *envp[]) {
    return exec(nextpid(), file_name, argv, envp);
}

long sys_execve(const char *file_name, const char *argv[], const char *envp[]) {
    int ret;
    ret = exec((*current_running)->pid, file_name, argv, envp);
    if (ret) {
        k_scheduler();
    }
    return ret;
}

long sys_clone(unsigned long flags, void *stack, int (*fn)(void *arg), void *arg, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    int i = nextpid();
    pid_t fpid = (*current_running)->pid;

    char name[NUM_MAX_PCB_NAME];
    int name_len = k_min(k_strlen((*current_running)->name), 14);
    k_memcpy((uint8_t *)name, (uint8_t *)(*current_running)->name, name_len);
    k_strcat(name, "_child");
    init_pcb_i(name, i, USER_PROCESS, i, i, 0, fpid, (*current_running)->core_mask[0]);
    pcb[i].cursor_x = (*current_running)->cursor_x;
    pcb[i].cursor_y = (*current_running)->cursor_y;

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
    ptr_t user_stack_kva = kernel_stack_kva - PAGE_SIZE;

    if (flags & CLONE_VM) {
        pcb[i].pgdir = (*current_running)->pgdir;
        
    } else {
        PTE *pgdir = (PTE *)k_alloc_page(1);
        clear_pgdir((ptr_t)pgdir);
        
        fork_pgtable(pgdir, (pa2kva((*current_running)->pgdir << NORMAL_PAGE_SHIFT)));
        share_pgtable(pgdir, pa2kva(PGDIR_PA));
        pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    }
    
    k_memcpy((uint8_t *)&pcb[i].elf, (uint8_t *)&(*current_running)->elf, sizeof(ELF_info_t));

    ptr_t user_stack = (ptr_t)stack ? (ptr_t)stack : USER_STACK_ADDR;
    // ptr_t user_stack = USER_STACK_ADDR;
    clone_pcb_stack(kernel_stack_kva, user_stack, &pcb[i], flags, tls, fn, arg);
    if(!stack) {
        map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pa2kva(pcb[i].pgdir << NORMAL_PAGE_SHIFT));
    }
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long k_getpid(void) {
    return (*current_running)->fpid;
}

long sys_getpid() {
    return k_getpid();
}

long sys_getppid() {
    return (*current_running)->father_pid + 1;
}

void k_sleep(void *chan, spin_lock_t *lk) {
    k_spin_lock_release(lk);
    (*current_running)->chan = chan;
    k_block(&(*current_running)->list, &block_queue, ENQUEUE_LIST);
    k_scheduler();
    (*current_running)->chan = 0;
    k_spin_lock_acquire(lk);
}

void k_wakeup(void *chan) {
    for (int i = 0; i < NUM_MAX_PROCESS; i++) {
        if ((pcb[i].status == TASK_BLOCKED) && (pcb[i].chan == chan)) {
            k_unblock(&pcb[i].list, &ready_queue, UNBLOCK_TO_LIST_STRATEGY);
        }
    }
}