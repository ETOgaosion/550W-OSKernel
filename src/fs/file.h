#pragma once

#include <common/types.h>
#include <lib/list.h>
#include <lib/math.h>
#include <lib/ring_buffer.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/mm.h>
// #include <os/errno.h>
// #include <os/pcb.h>
// flags
#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR 0x02 // 可读可写
// #define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define AT_FDCWD -100
#define AT_REMOVEDIR 0x200 /* Remove directory instead of unlinking file.  */

#define MAX_NAME_LEN 256

typedef uint32_t fd_num_t;
typedef uint32_t pipe_num_t;
typedef uint64_t dev_t;
typedef uint64_t ino_t;
typedef uint32_t mode_t;
typedef uint32_t nlink_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int64_t off_t;

typedef struct fd {
    list_head list;
    // file or dir?
    uint8_t file;
    /* file or dir name */
    char name[MAX_NAME_LEN];
    /* dev number */
    uint8_t dev;
    /* first clus number */
    uint32_t first_cluster;
    /*inode num*/
    ptr_t inode;
    /* page cache addr*/
    ptr_t mapping;
    /* file mode */
    uint32_t mode;
    /* open flags */
    uint32_t flags;
    /* position */
    uint64_t pos;
    /* length */
    uint32_t size;

    /* fd number */
    fd_num_t fd_num;

    /* used */
    uint8 used;
    /* redirect */
    uint8 redirected;
    uint8 redirected_fd_index;

    /* pipes[pip_num] is the pipe */
    pipe_num_t pip_num;
    uint8 is_pipe_read;
    uint8 is_pipe_write;
    /* socket */
    uint8 sock_num;
    /*mmap*/
    struct {
        int used;
        void *start;
        size_t len;
        int prot;
        int flags;
        off_t off;
    } mmap;
    /* fcntl */
    // flock_t flock;
    /* links */
    uint8 nlink;

    // /* status */
    uid_t uid;
    gid_t gid;

    dev_t rdev;

    // blksize_t blksize;

    long atime_sec;
    long atime_nsec;
    long mtime_sec;
    long mtime_nsec;
    long ctime_sec;
    long ctime_nsec;

    int mailbox;

} fd_t;

// fd func
#define MAX_FD 256
extern fd_t fd_table[MAX_FD];

extern int fd_table_init();
extern int fd_alloc(int fd);
extern int fd_free(int fd);
extern int pipe_alloc(int *fd);
extern fd_t *get_fd(int fd);
extern fd_t *fd_exist(int fd);

typedef struct pipe {
    // mutex_lock_t lock;
    int mailbox;
    pid_t pid; // parent id
    list_head r_list;
    list_head w_list;
    uint8 r_valid;
    uint8 w_valid;
} pipe_t;
#define PIPE_NUM 200
extern pipe_t pipe_table[PIPE_NUM];
extern int pipe_alloc(int *fd);