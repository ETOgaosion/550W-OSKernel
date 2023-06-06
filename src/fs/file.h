#pragma once

#include <common/types.h>
#include <lib/list.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/mm.h>
// #include <os/errno.h>

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
    // file or dir?
    uint8_t file;
    /* file or dir name */
    char name[MAX_NAME_LEN];
    /* dev number */
    uint8_t dev;
    /* first clus number */
    uint32_t first_cluster;
    /* file mode */
    uint32_t mode;
    /* open flags */
    uint32_t flags;
    /* position */
    uint64_t pos;
    uint32_t cur_clus_num;
    /* length */
    uint32_t size;
    /* dir_pos */
    // dir_pos_t dir_pos;

    /* fd number */
    /* default: its index in fd-array*/
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

    // share fd
    //  struct fd* share_fd;
    //  int   share_num;
    //  long version;
} fd_t;

// fd func
#define MAX_FD 200
extern fd_t fd_table[MAX_FD];

extern int fd_table_init();
extern int fd_alloc();
fd_t *fd_alloc_spec(int fd);
extern int fd_free(int fd);
extern fd_t *get_fd(int fd);

#define RING_BUFFER_SIZE 4095
#pragma pack(8)
struct ring_buffer {
    spin_lock_t lock; /* FOR NOW no use */
    size_t size;      // for future use
    int32_t head;     // read from head
    int32_t tail;     // write from tail
    // char buf[RING_BUFFER_SIZE + 1]; // left 1 byte
    uint8_t *buf;
};
#pragma pack()

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

void ring_buffer_init(struct ring_buffer *rbuf);
// static inline void ring_buffer_init(struct ring_buffer *rbuf) {
//     // there is always one byte which should not be read or written
//     k_memset(rbuf, 0, sizeof(struct ring_buffer)); /* head = tail = 0 */
//     rbuf->size = RING_BUFFER_SIZE;
//     rbuf->buf = k_malloc(PAGE_SIZE);
//     k_memset(rbuf->buf, 0, PAGE_SIZE);
//     // TODO INIT LOCK
//     k_spin_lock_init(&rbuf->lock);
//     return;
// }

static inline int ring_buffer_used(struct ring_buffer *rbuf) {
    return (rbuf->tail - rbuf->head + rbuf->size) % (rbuf->size);
}

static inline int ring_buffer_free(struct ring_buffer *rbuf) {
    // let 1 byte to distinguish empty buffer and full buffer
    return rbuf->size - ring_buffer_used(rbuf) - 1;
}

static inline int ring_buffer_empty(struct ring_buffer *rbuf) {
    return ring_buffer_used(rbuf) == 0;
}

static inline int ring_buffer_full(struct ring_buffer *rbuf) {
    return ring_buffer_free(rbuf) == 0;
}

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

size_t read_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size);
// static inline size_t read_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size) {
//     // TODO pcb and lock
//     k_spin_lock_acquire(&rbuf->lock);
//     int32_t len = min(ring_buffer_used(rbuf), size);
//     if (len > 0) {
//         if (rbuf->head + len > rbuf->size) {
//             int32_t right = rbuf->size - rbuf->head, left = len - right;
//             k_memcpy(buf, rbuf->buf + rbuf->head, right);
//             k_memcpy(buf + right, rbuf->buf, left);
//         } else {
//             k_memcpy(buf, rbuf->buf + rbuf->head, len);
//         }

//         rbuf->head = (rbuf->head + len) % (rbuf->size);
//     }
//     k_spin_lock_release(&rbuf->lock);
//     return len;
// }

// rbuf should have enough space for buf
size_t write_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size);
// static inline size_t write_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size) {
//     // TODO SYNC BY LOCK
//     k_spin_lock_acquire(&rbuf->lock);
//     int32_t len = min(ring_buffer_free(rbuf), size);
//     if (len > 0) {
//         if (rbuf->tail + len > rbuf->size) {
//             int32_t right = rbuf->size - rbuf->tail, left = len - right;
//             k_memcpy(rbuf->buf + rbuf->tail, buf, right);
//             if (left > 0) {
//                 k_memcpy(rbuf->buf, buf + right, left);
//             }
//         } else {
//             k_memcpy(rbuf->buf + rbuf->tail, buf, len);
//         }

//         rbuf->tail = (rbuf->tail + len) % (rbuf->size);
//     }
//     k_spin_lock_release(&rbuf->lock);
//     return len;
// }