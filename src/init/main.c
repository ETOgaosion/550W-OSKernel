#include <asm/common.h>
#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <asm/stack.h>
#include <asm/syscall.h>
#include <common/elf.h>
#include <drivers/screen.h>
#include <fs/fs.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <os/irq.h>
#include <os/lock.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/smp.h>
#include <user/user_programs.h>

int freepidnum = 0;
int nowpid = 1;
int glmask = 3;
int glvalid = 0;

pid_t freepid[NUM_MAX_TASK];
typedef void (*void_task)();
pid_t nextpid() {
    if (freepidnum != 0)
        return freepid[freepidnum--];
    else
        return nowpid++;
}

void tostr(char *s, int num) {
    int i = num;
    int len = 0;
    char temp;
    do {
        s[len++] = (i % 10) + '0';
        i /= 10;
    } while (i != 0);
    for (i = 0; i < len / 2; i++) {
        temp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = temp;
    }
    s[len] = '\0';
}

int ps() {
    int num = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        if (pcb[i].status != TASK_EXITED) {
            if (num == 0)
                prints("[PROCESS TABLE]\n");
            prints("[%d] PID : %d, TID : %d", num, pcb[i].fpid,
                   pcb[i].tid); // pcb[i].tid
            switch (pcb[i].status) {
            case TASK_RUNNING:
                screen_write(" STATUS : RUNNING");
                break;
            case TASK_BLOCKED:
                screen_write(" STATUS : BLOCKED");
                break;
            case TASK_ZOMBIE:
                screen_write(" STATUS : ZOMBIE");
                break;
            case TASK_READY:
                screen_write(" STATUS : READY");
                break;
            default:
                break;
            }
            prints(" MASK : 0x%d", pcb[i].cpuid);
            if (pcb[i].status == TASK_RUNNING) {
                screen_write(" on Core ");
                switch (pcb[i].cpuid) {
                case 1:
                    screen_write("0\n");
                    break;
                case 2:
                    screen_write("1\n");
                    break;
                case 3:
                    if (&pcb[i] == current_running0)
                        screen_write("0\n");
                    else if (&pcb[i] == current_running1)
                        screen_write("1\n");
                    break;
                default:
                    break;
                }
            } else
                screen_write("\n");
            num++;
        }
    }
    return num + 1;
}

int fs() {
    prints("[EXECABLE LIST]\n");
    /* TODO: develop new program loading model */
    for (int i = 1; i < ELF_FILE_NUM; i++) {
        prints("%d: %s", i, elf_files[i].file_name);
        screen_write("\n");
    }
    return ELF_FILE_NUM;
}

int do_kill(pid_t pid) {
    if (pcb[pid].status == TASK_EXITED || pid == 0)
        return 0;
    if (pcb[pid].status == TASK_RUNNING || current_running0 == &pcb[pid] || current_running1 == &pcb[pid]) {
        pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
        pcb[pid].killed = 1;
        return 1;
    }
    for (int i = 0; i < pcb[pid].locksum; i++) {
        do_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for (int i = 0; i < pcb[pid].threadsum; i++) {
        do_kill(pcb[pid].threadid[i]);
        // prints("kill %d\n", pcb[pid].threadid[i]);
    }
    pcb[pid].status = TASK_EXITED;
    // get back stack
    freepidnum++;
    freepid[freepidnum] = pid;
    if (pcb[pid].type != USER_THREAD)
        getback_page(pid);
    while (pcb[pid].wait_list.next != &pcb[pid].wait_list)
        do_unblock(&pcb[pid].wait_list, UNBLOCK_FROM_QUEUE);
    if (ready_queue == &pcb[pid].list)
        ready_queue = ready_queue->next;
    if (ready_queue0 == &pcb[pid].list)
        ready_queue0 = ready_queue0->next;
    if (ready_queue1 == &pcb[pid].list)
        ready_queue1 = ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    return 1;
}

void do_exit() {
    int id = get_current_cpu_id();
    current_running = id == 0 ? current_running0 : current_running1;
    pid_t pid = current_running->pid;
    // shell not allowed
    if (pid == 0)
        return;
    pcb[pid].killed = 0;
    for (int i = 0; i < pcb[pid].locksum; i++) {
        do_mutex_lock_release(pcb[pid].lockid[i]);
    }
    for (int i = 0; i < pcb[pid].threadsum; i++) {
        // prints("exit %d\n", pcb[pid].threadid[i]);
        do_kill(pcb[pid].threadid[i]);
        do_waitpid(pcb[pid].threadid[i]);
    }
    if (pcb[pid].mode == ENTER_ZOMBIE_ON_EXIT) {
        pcb[pid].status = TASK_ZOMBIE;
    } else if (pcb[pid].mode == AUTO_CLEANUP_ON_EXIT) {
        pcb[pid].status = TASK_EXITED;
        freepidnum++;
        freepid[freepidnum] = pid;
        if (pcb[pid].type != USER_THREAD)
            getback_page(pid);
    }
    while (pcb[pid].wait_list.next != &pcb[pid].wait_list)
        do_unblock(&pcb[pid].wait_list, UNBLOCK_FROM_QUEUE);
    if (ready_queue == &pcb[pid].list)
        ready_queue = ready_queue->next;
    if (ready_queue0 == &pcb[pid].list)
        ready_queue0 = ready_queue0->next;
    if (ready_queue1 == &pcb[pid].list)
        ready_queue1 = ready_queue1->next;
    pcb[pid].list.prev->next = pcb[pid].list.next;
    pcb[pid].list.next->prev = pcb[pid].list.prev;
    if (id == 0)
        current_running0 = &pid0_pcb;
    else if (id == 1)
        current_running1 = &pid0_pcb2;
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    do_scheduler();
}

int do_waitpid(pid_t pid) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    while (pcb[pid].status != TASK_EXITED && pcb[pid].status != TASK_ZOMBIE) {
        do_block(&current_running->list, &pcb[pid].wait_list);
        do_scheduler();
    }
    if (pcb[pid].status == TASK_ZOMBIE) {
        freepidnum++;
        freepid[freepidnum] = pid;
        pcb[pid].status = TASK_EXITED;
    }
    return 1;
}

pid_t getpid() {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    return current_running->pid;
}

int do_taskset(int pid, int mask) {
    int id = get_current_cpu_id();
    if (pid == 0) {
        if (mask != 3 && mask != 1 + id) {
            glmask = mask;
            glvalid = 1;
            return 1;
        } else
            glvalid = 0;
    }
    if (pid > NUM_MAX_TASK || pcb[pid].status == TASK_EXITED || pcb[pid].status == TASK_ZOMBIE)
        return 0;
    pcb[pid].cpuid = mask;
    return 1;
}

// can shu you dian shao
pid_t spawn(task_info_t *info, void *arg, spawn_mode_t mode) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int i = nextpid();
    list_node_t *p;
    pcb[i].type = info->type;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    pcb[i].mode = mode;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].pgdir = 0;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    pcb[i].locksum = 0;
    ptr_t kernel_stack = get_kernel_address(i);
    ptr_t user_stack = kernel_stack - 0x1000;
    /* TODO: use glibc argument passing standards to restructure */
    /* 1, arg is wrong, only for passing compilation */
    init_pcb_stack(kernel_stack, user_stack, (uintptr_t)(info->entry_point), &pcb[i], (uint64_t)arg, NULL);
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    return i;
}

int mthread_create(pid_t *thread, void (*start_routine)(void *), void *arg) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i = nextpid();

    pcb[fpid].threadid[pcb[fpid].threadsum] = i;
    pcb[fpid].threadsum++;
    pcb[i].father_pid = i;
    pcb[i].tid = pcb[fpid].threadsum;
    pcb[i].fpid = fpid;
    list_node_t *p;
    pcb[i].type = USER_THREAD;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    // pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
    ptr_t kernel_stack_kva = get_kernel_address(i);
    ptr_t user_stack_kva = kernel_stack_kva - 0x2000;
    uint64_t user_stack_uva = USER_STACK_ADDR + 0x1000 * pcb[fpid].threadsum + 0x1000;
    pcb[i].pgdir = pcb[fpid].pgdir;
    map(user_stack_uva - 0x1000, kva2pa(user_stack_kva), (pa2kva(pcb[i].pgdir << 12)));
    init_pcb_stack(kernel_stack_kva, (ptr_t)user_stack_uva, (ptr_t)start_routine, &pcb[i], (uint64_t)arg, NULL);
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    *thread = i;
    return 1;
}

extern int try_get_from_file(const char *file_name, unsigned char **binary, int *length);
pid_t exec(char *name, int argc, char **argv) {
    int len = 0;
    unsigned char *binary = NULL;
    /* TODO: use FAT32 disk to read program */
    if (get_elf_file(name, &binary, &len) == 0) {
        if (try_get_from_file(name, &binary, &len) == 0)
            return 0;
    }
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int i = nextpid();
    list_node_t *p;
    pcb[i].fpid = i;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].tid = 0;
    pcb[i].list.pid = i;
    pcb[i].threadsum = 0;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    pcb[i].priority = 0;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    pcb[i].father_pid = i;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;
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
    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // ready_queue=p;
    return i;
}

void init_net() { printk("[net] Init Net is not neccessary\n\r"); }

extern void map_fs_space();
extern void init_fs();

list_node_t send_queue;
list_node_t recv_queue;

static void init_pcb() {
    allocpid = 0;
    init_fs();
    init_net();
    for (int i = 0; i < NUM_MAX_TASK; i++) {
        pcb[i].status = TASK_EXITED;
        pcb[i].first = 1;
    }
    pcb[0].type = USER_PROCESS;
    pcb[0].pid = 0;
    pcb[0].fpid = 0;
    pcb[0].tid = 0;
    pcb[0].threadsum = 0;
    pcb[0].list.pid = 0;
    pcb[0].list.next = &pcb[0].list;
    pcb[0].list.prev = &pcb[0].list;
    pcb[0].status = TASK_READY;
    pcb[0].cursor_x = 1;
    pcb[0].cursor_y = 1;
    pcb[0].priority = 0;
    pcb[0].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[0].wait_list.next = &pcb[0].wait_list;
    pcb[0].wait_list.prev = &pcb[0].wait_list;
    pcb[0].cpuid = 3;
    pcb[0].locksum = 0;
    pcb[0].child_num = 0;
    pcb[0].killed = 0;
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
    ready_queue = &pcb[0].list;
    ready_queue0 = &pcb[0].list;
    ready_queue1 = &pcb[0].list;
    current_running1 = &pid0_pcb2;
    timers.next = &timers;
    timers.prev = &timers;
    send_queue.next = &send_queue;
    send_queue.prev = &send_queue;
    recv_queue.next = &recv_queue;
    recv_queue.prev = &recv_queue;
}

static int do_fork(int prior) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    pid_t fpid = current_running->pid;
    int i = nextpid();

    pcb[fpid].child_pid[pcb[fpid].child_num] = i;
    pcb[fpid].child_num++;
    pcb[i].father_pid = fpid;
    pcb[i].tid = 0;
    pcb[i].fpid = i;
    list_node_t *p;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i;
    pcb[i].list.pid = i;
    pcb[i].status = TASK_READY;
    pcb[i].cursor_x = 1;
    pcb[i].cursor_y = 1;
    // pcb[i].priority = prior;
    pcb[i].priority = 0;
    pcb[i].mode = ENTER_ZOMBIE_ON_EXIT;
    // pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[i].wait_list.next = &pcb[i].wait_list;
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].first = 1;
    pcb[i].killed = 0;
    pcb[i].threadsum = 0;
    pcb[i].locksum = 0;
    pcb[i].child_num = 0;
    if (glvalid)
        pcb[i].cpuid = glmask;
    else
        pcb[i].cpuid = current_running->cpuid;

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

    p = &pcb[i].list;
    // add pcb to the ready
    ready_queue->prev->next = p;
    p->next = ready_queue;
    p->prev = ready_queue->prev;
    ready_queue->prev = p;
    // do_scheduler();
    return i;
}

void do_net_port(int id) {
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    if (id == 50001)
        current_running->port = 1;
    else if (id == 58688)
        current_running->port = 2;
}

static void init_syscall(void) {
    syscall[SYSCALL_SLEEP] = (long (*)())do_sleep;
    syscall[SYSCALL_YIELD] = (long (*)())do_scheduler;
    syscall[SYSCALL_READ] = (long (*)())port_read;
    syscall[SYSCALL_WRITE] = (long (*)())screen_write;
    syscall[SYSCALL_FORK] = (long (*)())do_fork;
    syscall[SYSCALL_CURSOR] = (long (*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (long (*)())screen_reflush;
    syscall[SYSCALL_GET_TICK] = (long (*)())get_ticks;
    syscall[SYSCALL_GET_TIMEBASE] = (long (*)())get_time_base;
    syscall[SYSCALL_LOCK_INIT] = (long (*)())do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQUIRE] = (long (*)())do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE] = (long (*)())do_mutex_lock_release;
    syscall[SYSCALL_GET_WALL_TIME] = (long (*)())get_wall_time;
    syscall[SYSCALL_CLEAR] = (long (*)())screen_clear;
    syscall[SYSCALL_PS] = (long (*)())ps;
    syscall[SYSCALL_SPAWN] = (long (*)())spawn;
    syscall[SYSCALL_KILL] = (long (*)())do_kill;
    syscall[SYSCALL_EXIT] = (long (*)())do_exit;
    syscall[SYSCALL_WAITPID] = (long (*)())do_waitpid;
    syscall[SYSCALL_GETPID] = (long (*)())getpid;
    syscall[SYSCALL_SEMAPHORE_INIT] = (long (*)())do_semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP] = (long (*)())do_semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN] = (long (*)())do_semaphore_down;
    syscall[SYSCALL_SEMAPHORE_DESTROY] = (long (*)())do_semaphore_destroy;
    syscall[SYSCALL_BARRIER_INIT] = (long (*)())do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = (long (*)())do_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY] = (long (*)())do_barrier_destroy;
    syscall[SYSCALL_MBOX_OPEN] = (long (*)())do_mbox_open;
    syscall[SYSCALL_MBOX_SEND] = (long (*)())do_mbox_send;
    syscall[SYSCALL_MBOX_CLOSE] = (long (*)())do_mbox_close;
    syscall[SYSCALL_MBOX_RECV] = (long (*)())do_mbox_recv;
    syscall[SYSCALL_TASKSET] = (long (*)())do_taskset;
    syscall[SYSCALL_TEST_RECV] = (long (*)())do_test_recv;
    syscall[SYSCALL_TEST_SEND] = (long (*)())do_test_send;
    syscall[SYSCALL_EXEC] = (long (*)())exec;
    syscall[SYSCALL_FS] = (long (*)())fs;
    syscall[SYSCALL_MTHREAD_CREATE] = (long (*)())mthread_create;
    syscall[SYSCALL_GETPA] = (long (*)())do_getpa;
    syscall[SYSCALL_SHMPAGE_GET] = (long (*)())shm_page_get;
    syscall[SYSCALL_SHMPAGE_DT] = (long (*)())shm_page_dt;
    syscall[SYSCALL_NET_PORT] = (long (*)())do_net_port;
    syscall[SYSCALL_MKFS] = (long (*)())do_mkfs;
    syscall[SYSCALL_STATFS] = (long (*)())do_statfs;
    syscall[SYSCALL_CD] = (long (*)())do_cd;
    syscall[SYSCALL_MKDIR] = (long (*)())do_mkdir;
    syscall[SYSCALL_RMDIR] = (long (*)())do_rmdir;
    syscall[SYSCALL_LS] = (long (*)())do_ls;
    syscall[SYSCALL_TOUCH] = (long (*)())do_touch;
    syscall[SYSCALL_CAT] = (long (*)())do_cat;
    syscall[SYSCALL_FOPEN] = (long (*)())do_fopen;
    syscall[SYSCALL_FREAD] = (long (*)())do_fread;
    syscall[SYSCALL_FWRITE] = (long (*)())do_fwrite;
    syscall[SYSCALL_FCLOSE] = (long (*)())do_fclose;
    syscall[SYSCALL_LN] = (long (*)())do_ln;
    syscall[SYSCALL_RM] = (long (*)())do_rm;
    syscall[SYSCALL_LSEEK] = (long (*)())do_lseek;
    // initialize system call table.
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main() {
    int id = get_current_cpu_id();
    if (id == 1) {
        setup_exception();
        cancelpg(pa2kva(PGDIR_PA));
        sbi_set_timer(0);
    }
    // init Process Control Block (-_-!)
    init_pcb();
    // printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // init system call table (0_0)
    init_syscall();
    // printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);
    init_exception();
    // printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init screen (QAQ)
    init_screen();
    // printk("> [INIT] SCREEN initialization succeeded.\n\r");
    // Setup timer interrupt and enable all interrupt
    // init interrupt (^_^)
    setup_exception();
    sbi_set_timer(0);
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        // do_scheduler();
    };
    return 0;
}
