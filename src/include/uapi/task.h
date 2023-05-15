#pragma once

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

/* task information, used to init PCB */
typedef struct task_info
{
    uintptr_t entry_point;
    task_type_t type;
} task_info_t;

typedef int32_t pid_t;

/* define for test_fs.c */

#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3 /* read/write open */