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
#include <os/signal.h>
#include <os/smp.h>
#include <os/users.h>
#include <user/user_programs.h>

#include<fs/file.h>

pcb_t *volatile current_running0;
pcb_t *volatile current_running1;
pcb_t *volatile *volatile current_running;

LIST_HEAD(ready_queue);
LIST_HEAD(block_queue);

pcb_t pcb[NUM_MAX_TASK];

const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE * 3 - 112 - 288;
const ptr_t pid0_stack2 = INIT_KERNEL_STACK + PAGE_SIZE * 5 - 112 - 288;
pcb_t pid0_pcb = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack, .user_sp = (ptr_t)(INIT_KERNEL_STACK + PAGE_SIZE * 2), .core_mask[0] = 0x3, .status = TASK_EXITED, .sched_policy = DEQUEUE_LIST_FIFO};
pcb_t pid0_pcb2 = {.pid = -1, .kernel_sp = (ptr_t)pid0_stack2, .user_sp = (ptr_t)(INIT_KERNEL_STACK + PAGE_SIZE * 4), .core_mask[0] = 0x3, .status = TASK_EXITED, .sched_policy = DEQUEUE_LIST_FIFO};

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

/* ============== kernel PCB operation ============== */

/**
 * @brief Intitialization of pcb fields except stacks
 *
 * @param name
 * @param pcb_i
 * @param type
 * @param pid
 * @param fpid
 * @param tid
 * @param father_pid
 * @param core_mask
 */
void init_pcb_i(char *name, int pcb_i, task_type_t type, int pid, int tid, int uid, int father_pid, uint8_t core_mask) {
    pcb_t *target = &pcb[pcb_i];
    k_bzero((uint8_t *)target->name, sizeof(target->name));
    k_memcpy((uint8_t *)target->name, (uint8_t *)name, k_strlen(name));
    target->in_use = TRUE;
    target->pid = pid;
    target->tid = tid;
    if (type == USER_THREAD) {
        target->pgid = father_pid;
        target->gid.rgid = father_pid;
        target->gid.egid = father_pid;
        target->gid.sgid = father_pid;
    } else {
        target->pgid = pid;
        target->gid.rgid = pid;
        target->gid.egid = pid;
        target->gid.sgid = pid;
    }
    target->uid.ruid = uid;
    target->uid.euid = uid;
    target->uid.suid = uid;
    target->father_pid = father_pid;
    target->child_num = 0;
    k_bzero((void *)target->child_pids, sizeof(target->child_pids));
    k_bzero((void *)target->child_stat_addrs, sizeof(target->child_stat_addrs));
    target->threadsum = 0;
    k_bzero((void *)target->thread_ids, sizeof(target->child_pids));
    target->type = type;
    target->status = TASK_READY;
    target->cursor_x = 1;
    target->cursor_y = 1;
    target->sched_policy = SCHED_OTHER;
    target->priority.priority = 1;
    target->priority.last_sched_time = 0;
    target->core_mask[0] = core_mask;
    target->personality = 0;
    target->handling_signal = false;
    target->sigactions = NULL;
    target->sig_recv = 0;
    target->sig_pend = 0;
    target->sig_mask = 0;
    target->prev_mask = 0;
    target->locksum = 0;
    target->mbox = &pcb_mbox[pcb_i];
    k_pcb_mbox_init(target->mbox, pcb_i);
    k_bzero((void *)&(target->stime_last), sizeof(kernel_time_t));
    k_bzero((void *)&(target->utime_last), sizeof(kernel_time_t));
    k_time_get_utime(&target->real_time_last);
    k_bzero((void *)&(target->timer), sizeof(pcbtimer_t));
    k_bzero((void *)&(target->real_itime), sizeof(kernel_itimerval_t));
    k_bzero((void *)&(target->virt_itime), sizeof(kernel_itimerval_t));
    k_bzero((void *)&(target->prof_itime), sizeof(kernel_itimerval_t));
    k_time_get_nanotime(&target->real_time_last_nano);
    k_bzero((void *)&(target->cputime_id_clock), sizeof(clock_t));
    target->dead_child_stime = 0;
    target->dead_child_utime = 0;
    k_bzero((void *)&(target->resources), sizeof(rusage_t));
    target->resources.ru_maxrss = PAGE_SIZE + STACK_SIZE;
    target->resources.ru_idrss = PAGE_SIZE;
    target->resources.ru_isrss = STACK_SIZE;
    k_bzero((void *)&(target->cap), 2 * sizeof(cap_user_data_t));
}

uintptr_t init_user_stack(pcb_t *new_pcb, ptr_t *user_stack_kva, ptr_t *user_stack, int argc, const char *argv[], int envpc, const char *envp[], const char *file_name) {
    int total_length = (argc + envpc + 3) * sizeof(uintptr_t *) + sizeof(aux_elem_t) * (AUX_CNT + 1) + SIZE_RESTORE;
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
    uintptr_t file_pointer = new_argv;
    new_argv = new_argv + k_strlen(file_name) + 1;
    kargv += k_strlen(file_name) + 1;

    /* 5. set aux */
    aux_elem_t aux_vec[AUX_CNT];
    set_aux_vec((aux_elem_t *)kargv_pointer, &new_pcb->elf, file_pointer, new_argv);

    /* 6. copy aux on the user_stack */
    k_memcpy((uint8_t *)kargv_pointer, (const uint8_t *)aux_vec, sizeof(aux_elem_t) * (AUX_CNT + 1));
    *((uintptr_t *)kargv_pointer + AUX_CNT + 1) = 0;

    /* now user_sp is user_stack - total_length */
    *user_stack_kva -= total_length;
    *user_stack -= total_length;
    return ret;
}

void k_pcb_init() {
    k_bzero(&pcb, sizeof(pcb));
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].pid = -1;
        pcb[i].status = TASK_EXITED;
    }
    current_running0 = &pid0_pcb;
    current_running1 = &pid0_pcb2;
    current_running = k_smp_get_current_running();
    init_list_head(&timers);
}

long k_pcb_getpid(void) {
    return (*current_running)->pid;
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

pcb_t *choose_sched_task(list_head *queue, int policy) {
    uint64_t cur_time = k_time_get_ticks();
    uint64_t max_priority = 0;
    uint64_t cur_priority = 0;
    list_head *list_iterator = queue->next;
    pcb_t *max_one = NULL;
    pcb_t *pcb_iterator = NULL;
    int cpu_id = k_smp_get_current_cpu_id();
    if (policy == SCHED_RR || policy == SCHED_IDLE) {
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
    } else if (policy == SCHED_FIFO) {
        while (list_iterator->next != queue) {
            pcb_iterator = list_entry(list_iterator, pcb_t, list);
            if (!(pcb_iterator->core_mask[cpu_id / 8] & (0x1 << (cpu_id % 8)))) {
                continue;
            }
            if (pcb_iterator->priority.priority > (*current_running)->priority.priority) {
                max_one = pcb_iterator;
                break;
            }
            if (pcb_iterator->priority.priority <= (*current_running)->priority.priority) {
                max_one = *current_running;
                break;
            }
        }
    }
    return max_one;
}

void enqueue(list_head *new, list_head *head, enqueue_way_t way) {
    switch (way) {
    case ENQUEUE_LIST:
        list_add_tail(new, head);
        break;
    case ENQUEUE_LIST_PRIORITY: {
        pcb_t *new_inserter = list_entry(new, pcb_t, list);
        list_head *iterator_list = ready_queue.next;
        pcb_t *iterator_pcb = NULL;
        while (iterator_list != &ready_queue) {
            iterator_pcb = list_entry(iterator_list, pcb_t, list);
            iterator_list = iterator_list->next;
            if (iterator_pcb->priority.priority < new_inserter->priority.priority) {
                break;
            }
        }
        list_add(new, iterator_list);
        break;
    }
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

pcb_t *dequeue(list_head *queue, dequeue_way_t target, int policy) {
    // plain and simple way
    pcb_t *ret = NULL;
    pcb_t *task = check_first_ready_task();
    if (list_is_empty(queue)) {
        return NULL;
    }
    switch (target) {
    case DEQUEUE_LIST:
        ret = list_entry(queue->next, pcb_t, list);
        list_del(&(ret->list));
        break;
    case DEQUEUE_LIST_FIFO:
        if (!task) {
            ret = list_entry(queue->next, pcb_t, list);
        } else {
            ret = task;
        }
        list_del(&(ret->list));
        break;
    case DEQUEUE_LIST_PRIORITY:
        ret = choose_sched_task(queue, policy);
        list_del(&(ret->list));
        break;
    default:
        break;
    }
    return ret;
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

void check_itimers() {
    if ((*current_running)->status == TASK_EXITED) {
        return;
    }
    /* update real_time */
    if (k_time_cmp_utime_0(&(*current_running)->real_itime.it_value)) {
        kernel_timeval_t now;
        k_time_get_utime(&now);
        kernel_timeval_t last_run;
        k_time_minus_utime(&now, &(*current_running)->real_time_last, &last_run);
        k_time_minus_utime(&(*current_running)->real_itime.it_value, &last_run, NULL);
    }
    /* update cputime_clock */
    nanotime_val_t now_nano;
    k_time_get_nanotime(&now_nano);
    nanotime_val_t last_run_nano;
    k_time_minus_nanotime(&now_nano, &(*current_running)->real_time_last_nano, &last_run_nano);
    k_time_add_nanotime((nanotime_val_t *)&(*current_running)->cputime_id_clock.nano_clock, &last_run_nano, NULL);
    /* update global clocks */
    k_time_add_nanotime((nanotime_val_t *)&global_clocks.real_time_clock, &last_run_nano, NULL);
    k_time_add_nanotime((nanotime_val_t *)&global_clocks.tai_clock, &last_run_nano, NULL);
    k_time_add_nanotime((nanotime_val_t *)&global_clocks.monotonic_clock, &last_run_nano, NULL);
    k_time_add_nanotime((nanotime_val_t *)&global_clocks.boot_time_clock, &last_run_nano, NULL);

    /* check timing */
    bool time_up = false;
    if (k_time_cmp_utime_0(&(*current_running)->real_itime.it_value) < 0) {
        k_send_signal(SIGALRM, *current_running);
        time_up = true;
    }
    if (k_time_cmp_utime_0(&(*current_running)->virt_itime.it_value) < 0) {
        k_send_signal(SIGVTALRM, *current_running);
        time_up = true;
    }
    if (k_time_cmp_utime_0(&(*current_running)->prof_itime.it_value) < 0) {
        k_send_signal(SIGPROF, *current_running);
        time_up = true;
    }
    if (time_up) {
        if (k_time_cmp_utime_0(&(*current_running)->real_itime.it_interval) > 0) {
            k_time_copy_utime(&(*current_running)->real_itime.it_interval, &(*current_running)->real_itime.it_value);
        }
        if (k_time_cmp_utime_0(&(*current_running)->virt_itime.it_interval) > 0) {
            k_time_copy_utime(&(*current_running)->virt_itime.it_interval, &(*current_running)->virt_itime.it_value);
        }
        if (k_time_cmp_utime_0(&(*current_running)->prof_itime.it_interval) > 0) {
            k_time_copy_utime(&(*current_running)->prof_itime.it_interval, &(*current_running)->prof_itime.it_value);
        }
    }
}

long k_pcb_scheduler(bool voluntary) {
    if (!list_is_empty(&timers)) {
        check_sleeping();
    }
    check_itimers();
    k_signal_handler();
    pcb_t *curr = (*current_running);
    if (curr->sched_policy == SCHED_BATCH && curr->status == TASK_RUNNING) {
        return 0;
    }
    int cpuid = k_smp_get_current_cpu_id();
    if (curr->status == TASK_RUNNING && curr->pid >= 0) {
        enqueue(&curr->list, &ready_queue, ENQUEUE_LIST);
    }
    pcb_t *next_pcb = dequeue(&ready_queue, DEQUEUE_LIST_FIFO, curr->sched_policy);
    if (!next_pcb || next_pcb == curr) {
        return 0;
    }
    if (!voluntary) {
        (*current_running)->resources.ru_nivcsw++;
    }
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

void k_pcb_block(list_node_t *pcb_node, list_head *queue, enqueue_way_t way) {
    enqueue(pcb_node, queue, way);
    (*current_running)->status = TASK_BLOCKED;
}

void k_pcb_unblock(list_head *from_queue, list_head *to_queue, unblock_way_t way) {
    pcb_t *fetch_pcb = NULL;
    switch (way) {
    case UNBLOCK_TO_LIST_FRONT:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST, -1);
        list_add(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_TO_LIST_BACK:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST, -1);
        list_add_tail(&fetch_pcb->list, to_queue);
        break;
    case UNBLOCK_ONLY:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST, -1);
        break;
    case UNBLOCK_TO_LIST_STRATEGY:
        fetch_pcb = dequeue(from_queue->prev, DEQUEUE_LIST_FIFO, -1);
        enqueue(&fetch_pcb->list, to_queue, ENQUEUE_LIST_PRIORITY);
        break;
    default:
        break;
    }
    fetch_pcb->status = TASK_READY;
}

void k_pcb_sleep(void *chan, spin_lock_t *lk) {
    k_spin_lock_release(lk);
    (*current_running)->chan = chan;
    k_pcb_block(&(*current_running)->list, &block_queue, ENQUEUE_LIST);
    k_pcb_scheduler(false);
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

int k_pcb_count() {
    return nextpid() - 1;
}

long spawn(const char *file_name) {
    int i = nextpid();

    init_pcb_i((char *)file_name, i, USER_PROCESS, i, 0, 0, (*current_running)->father_pid, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack - STACK_SIZE;
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
    init_list_head(&pcb[i].fd_head);
    list_add_tail(&pcb[i].list, &ready_queue);
    return i;
}

long exec(int target_pid, int father_pid, const char *file_name, const char *argv[], const char *envp[]) {
    init_pcb_i((char *)file_name, target_pid, USER_PROCESS, target_pid, 0, 0, father_pid, (*current_running)->core_mask[0]);

    ptr_t kernel_stack = get_kernel_address(target_pid);
    ptr_t user_stack_kva = kernel_stack - STACK_SIZE;
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
    uintptr_t child_argv = init_user_stack(&pcb[target_pid], &user_stack_kva, &user_stack, argc, argv, envpc, envp, file_name);

    init_context_stack(kernel_stack, user_stack, argc, (char **)child_argv, (ptr_t)process, &pcb[target_pid]);

    init_list_head(&pcb[target_pid].fd_head);
    list_add_tail(&pcb[target_pid].list, &ready_queue);
    return target_pid;
}

long clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    int i = nextpid();
    pid_t fpid = (*current_running)->pid;

    char name[NUM_MAX_PCB_NAME] = {0};
    int name_len = k_min(k_strlen((*current_running)->name), 14);
    k_memcpy((uint8_t *)name, (uint8_t *)(*current_running)->name, name_len);
    k_strcat(name, "_child");
    init_pcb_i(name, i, USER_PROCESS, i, 0, 0, fpid, (*current_running)->core_mask[0]);

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
    ptr_t user_stack_kva = kernel_stack_kva - STACK_SIZE;

    if (flags & CLONE_VM) {
        pcb[i].pgdir = (*current_running)->pgdir;

    } else {
        PTE *pgdir = (PTE *)k_mm_alloc_page(1);
        clear_pgdir((ptr_t)pgdir);

        k_mm_fork_pgtable(pgdir, (pa2kva((*current_running)->pgdir << NORMAL_PAGE_SHIFT)));
        k_mm_share_pgtable(pgdir, pa2kva(PGDIR_PA));
        pcb[i].pgdir = kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT;
    }

    k_memcpy((uint8_t *)&pcb[i].elf, (uint8_t *)&(*current_running)->elf, sizeof(elf_info_t));
    k_memcpy((uint8_t *)(user_stack_kva - NORMAL_PAGE_SIZE), (const uint8_t *)(K_ROUNDDOWN((*current_running)->user_sp, NORMAL_PAGE_SIZE)), NORMAL_PAGE_SIZE);

    ptr_t user_stack = (ptr_t)stack ? (ptr_t)stack : USER_STACK_ADDR;
    // ptr_t user_stack = USER_STACK_ADDR;
    clone_pcb_stack(kernel_stack_kva, user_stack, &pcb[i], flags, tls);
    if (!stack) {
        k_mm_map(USER_STACK_ADDR - PAGE_SIZE, kva2pa(user_stack_kva - PAGE_SIZE), pa2kva(pcb[i].pgdir << NORMAL_PAGE_SHIFT));
    }
    list_add_tail(&pcb[i].list, &ready_queue);
    //cpy fd
    init_list_head(&pcb[i].fd_head);
    fd_t *pos;
    list_for_each_entry(pos,&pcb[fpid].fd_head,list){
        fd_t *file = k_mm_malloc(sizeof(fd_t));
        k_memcpy((uint8_t *)file,(const uint8_t *)pos,sizeof(fd_t));
        __list_add(&file->list,pcb[i].fd_head.prev,&pcb[i].fd_head);
    }
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

/* =============== syscalls =============== */
/* =============== PCB Initialization =============== */

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
    init_pcb_i(name, i, USER_PROCESS, i, 0, 0, fpid, (*current_running)->core_mask[0]);

    pcb[fpid].child_pids[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;

    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - STACK_SIZE;
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

long sys_exec(const char *file_name, const char *argv[], const char *envp[]) {
    return exec(nextpid(), (*current_running)->pid, file_name, argv, envp);
}

long sys_execve(const char *file_name, const char *argv[], const char *envp[]) {
    int ret;
    ret = exec((*current_running)->pid, (*current_running)->father_pid, file_name, argv, envp);
    if (ret) {
        k_pcb_scheduler(false);
    }
    return ret;
}

long sys_clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    return clone(flags, stack, parent_tid, tls, child_tid);
}

/* ============== PCB Elimination ============== */

long sys_kill(pid_t pid) {
    if (pid <= 0 || pid >= NUM_MAX_TASK) {
        return -EINVAL;
    }
    kill(pid, 0);
    pcb[pid].in_use = FALSE;
    if (pid == (*current_running)->pid) {
        k_pcb_scheduler(false);
    }
    return 0;
}

// [TODO]
long sys_tkill(pid_t pid, int sig) {
    if (pid <= 0 || pid > NUM_MAX_TASK || sig < 0 || sig > NUM_MAX_SIGNAL) {
        return -EINVAL;
    }
    if (pcb[pid].father_pid) {
        k_send_signal(SIGCHLD, &pcb[pcb[pid].father_pid]);
    }
    k_send_signal(sig, &pcb[pid]);
    return 0;
}

long sys_tgkill(pid_t tgid, pid_t pid, int sig) {
    if (pid <= 0 || pid > NUM_MAX_TASK || sig < 0 || sig > NUM_MAX_SIGNAL) {
        return -EINVAL;
    }
    if (pcb[pid].father_pid) {
        k_send_signal(SIGCHLD, &pcb[pcb[pid].father_pid]);
    }
    k_send_signal(sig, &pcb[pid]);
    return 0;
}

long sys_exit(int error_code) {
    kill((*current_running)->pid, error_code);

    current_running = k_smp_get_current_running();
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    k_pcb_scheduler(false);
    return 0;
}

long sys_exit_group(int error_code) {
    for (int i = 0; i < (*current_running)->threadsum; i++) {
        sys_kill((*current_running)->thread_ids[i]);
    }
    sys_exit((*current_running)->pid);
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
        k_pcb_scheduler(false);
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

/* ============= PCB functions ============== */
long sys_process_show() {
    int num = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].status != TASK_EXITED) {
            if (num == 0) {
                prints("\n[PROCESS TABLE]\n");
            }
            prints("[%d] PID : %d, TID : %d", num, pcb[i].pid,
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

long sys_nanosleep(nanotime_val_t *rqtp, nanotime_val_t *rmtp) {
    (*current_running)->timer.initialized = TRUE;
    k_time_get_nanotime(&(*current_running)->timer.start_time);
    k_time_add_nanotime(rqtp, &(*current_running)->timer.start_time, &(*current_running)->timer.end_time);
    (*current_running)->timer.remain_time = rmtp;
    k_pcb_block(&((*current_running)->list), &timers, ENQUEUE_TIMER_LIST);
    k_pcb_scheduler(false);
    return 0;
}

/* =============== PCB priority ===============*/

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

/* =============== PCB Scheduling ================ */
long sys_sched_setparam(pid_t pid, sched_param_t *param) {
    pcb[pid].priority.priority = param->sched_priority;
    return 0;
}

long sys_sched_setscheduler(pid_t pid, int policy, sched_param_t *param) {
    if (policy == SCHED_OTHER || policy == SCHED_BATCH || policy == SCHED_IDLE) {
        if (param->sched_priority != 0) {
            return -EINVAL;
        }
    }
    if (policy == SCHED_IDLE) {
        param->sched_priority = 0;
    }
    pcb[pid].sched_policy = policy;
    pcb[pid].priority.priority = param->sched_priority;
    return 0;
}

long sys_sched_getscheduler(pid_t pid) {
    if (pid == 0) {
        return (*current_running)->sched_policy;
    }
    if (pid >= NUM_MAX_TASK) {
        return -EINVAL;
    }
    return pcb[pid].sched_policy;
}

long sys_sched_getparam(pid_t pid, sched_param_t *param) {
    param->sched_priority = pcb[pid].priority.priority;
    return 0;
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

long sys_sched_yield(void) {
    (*current_running)->resources.ru_nvcsw++;
    return k_pcb_scheduler(true);
}

long sys_sched_get_priority_max(int policy) {
    int max_priority = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].in_use && pcb[i].sched_policy == policy) {
            if (pcb[i].priority.priority > max_priority) {
                max_priority = pcb[i].priority.priority;
                break;
            }
        }
    }
    return max_priority;
}

long sys_sched_get_priority_min(int policy) {
    int min_priority = 0;
    for (int i = NUM_MAX_TASK - 1; i >= 0; i--) {
        if (pcb[i].in_use && pcb[i].sched_policy == policy) {
            min_priority = pcb[i].priority.priority;
            break;
        }
    }
    return min_priority;
}

/* =============== ids ===============*/
long sys_set_tid_address(int *tidptr) {
    (*current_running)->clear_ctid = (uint32_t *)tidptr;
    return (*current_running)->tid;
}

long sys_setgid(gid_t gid) {
    (*current_running)->gid.egid = gid;
    return 0;
}

long sys_setuid(uid_t uid) {
    (*current_running)->uid.euid = uid;
    return 0;
}

long sys_setresuid(uid_t ruid, uid_t euid, uid_t suid) {
    (*current_running)->uid.ruid = ruid;
    (*current_running)->uid.euid = euid;
    (*current_running)->uid.suid = suid;
    return 0;
}

long sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
    *ruid = (*current_running)->uid.ruid;
    *euid = (*current_running)->uid.euid;
    *suid = (*current_running)->uid.suid;
    return 0;
}

long sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
    (*current_running)->gid.rgid = rgid;
    (*current_running)->gid.egid = egid;
    (*current_running)->gid.sgid = sgid;
    return 0;
}

long sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
    *rgid = (*current_running)->gid.rgid;
    *egid = (*current_running)->gid.egid;
    *sgid = (*current_running)->gid.sgid;
    return 0;
}

long sys_setpgid(pid_t pid, pid_t pgid) {
    pcb_t *target = &pcb[pid];
    if (pid == 0) {
        target = *current_running;
    }
    pid_t new_pgid = pgid;
    if (pgid == 0) {
        new_pgid = (*current_running)->pgid;
    }
    target->pgid = new_pgid;
    return 0;
}

long sys_getpgid(pid_t pid) {
    if (pid == 0) {
        return (*current_running)->pgid;
    }
    return pcb[pid].pgid;
}

long sys_getsid(pid_t pid) {
    return pcb[pid].pgid;
}

long sys_setsid(void) {
    (*current_running)->pgid = (*current_running)->pid;
    return (*current_running)->pgid;
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

long sys_getuid(void) {
    return (*current_running)->uid.ruid;
}

long sys_geteuid(void) {
    return (*current_running)->uid.euid;
}

long sys_getgid(void) {
    return (*current_running)->gid.rgid;
}

long sys_getegid(void) {
    return (*current_running)->gid.egid;
}

long sys_gettid(void) {
    return (*current_running)->tid;
}

/* =============== PCB Resources ===============*/
long sys_getrusage(int who, rusage_t *ru) {
    if (who <= 0 || who >= NUM_MAX_TASK) {
        return -EINVAL;
    }
    k_memcpy((uint8_t *)ru, (const uint8_t *)&pcb[who].resources, sizeof(rusage_t));
    return 0;
}

/* [NOT CLEAR]: Why you ask process of these fields? */
long sys_prlimit64(pid_t pid, unsigned int resource, const rlimit64_t *new_rlim, rlimit64_t *old_rlim) {
    switch (resource) {
    case RLIMIT_CPU: {
        int cpu_num = 0, all_cpus = 0;
        for (int i = 0; i < CPU_SET_SIZE; i++) {
            int idx = 0x1;
            for (int j = 0; j < 8; j++) {
                if (all_cpus < new_rlim->rlim_cur) {
                    (*current_running)->core_mask[i] |= (idx << j);
                    all_cpus++;
                }
                if ((*current_running)->core_mask[i] & (idx << j)) {
                    cpu_num++;
                }
            }
        }
        old_rlim->rlim_cur = (__u64)cpu_num;
        old_rlim->rlim_max = (__u64)all_cpus;
        break;
    }
    case RLIMIT_RSS:
        k_set_rlimit((rlimit_t *)old_rlim, (*current_running)->resources.ru_maxrss, (*current_running)->resources.ru_maxrss);
        (*current_running)->resources.ru_maxrss = new_rlim->rlim_cur;
        (*current_running)->resources.ru_maxrss = new_rlim->rlim_max;
        break;
    case RLIMIT_LOCKS:
        k_set_rlimit((rlimit_t *)old_rlim, (*current_running)->locksum, (*current_running)->locksum);
        /* We don't support adjust lock num */
        break;
    case RLIMIT_DATA:
    case RLIMIT_STACK:
    case RLIMIT_FSIZE:
    case RLIMIT_CORE:
    case RLIMIT_NPROC:
    case RLIMIT_NOFILE:
    case RLIMIT_MEMLOCK:
    case RLIMIT_AS:
    case RLIMIT_SIGPENDING:
    case RLIMIT_MSGQUEUE:
    case RLIMIT_NICE:
    case RLIMIT_RTPRIO:
    case RLIMIT_RTTIME:
        sys_getrlimit(resource, (rlimit_t *)old_rlim);
        sys_setrlimit(resource, (rlimit_t *)new_rlim);
        break;

    default:
        break;
    }
    return 0;
}

/* ================ PCB Extra ==================*/
long sys_personality(unsigned int personality) {
    unsigned int old = (*current_running)->personality;
    if (personality != UINT_MAX) {
        (*current_running)->personality = personality;
    }
    return old;
}

/* clone need to support CLONE_FLAGS */
long sys_unshare(unsigned long unshare_flags) {
    k_print("unshare flag:%lu\n", unshare_flags);
    return 0;
}

long sys_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
    switch (option) {
    case PR_SET_NAME:
        k_strncpy((*current_running)->name, (const char *)arg2, k_strlen((*current_running)->name));
        break;
    case PR_GET_NAME:
        k_strncpy((char *)arg2, (*current_running)->name, k_strlen((*current_running)->name));
        break;

    default:
        k_print("prctl option:%i\n", option);
        break;
    }
    return 0;
}

long sys_capget(cap_user_header_t *header, cap_user_data_t *dataptr) {
    k_memcpy((uint8_t *)dataptr, (const uint8_t *)pcb[header->pid].cap, 2 * sizeof(cap_user_data_t));
    return 0;
}

long sys_capset(cap_user_header_t *header, const cap_user_data_t *data) {
    k_memcpy((uint8_t *)pcb[header->pid].cap, (const uint8_t *)data, 2 * sizeof(cap_user_data_t));
    return 0;
}