#include <fs/file.h>
#include <fs/fs.h>
// #include <os/time.h>
#include <os/pcb.h>
// #include <fs/pipe.h>

fd_t fd_table[MAX_FD];
pipe_t pipe_table[PIPE_NUM];
int pipe_cnt = 0;

int fd_table_init() {
    int i;
    for (i = STDMAX; i < MAX_FD; i++) {
        fd_table[i].used = 0;
    }
    // TODO : init std in/out/err
    fd_table[STDIN].file = STDIN;
    fd_table[STDIN].used = 1;
    fd_table[STDOUT].file = STDOUT;
    fd_table[STDOUT].used = 1;
    fd_table[STDERR].file = STDERR;
    fd_table[STDERR].used = 1;
    return 0;
}

int fd_alloc() {
    int i;
    for (i = STDMAX; i < MAX_FD; i++) {
        if (!fd_table[i].used) {
            k_memset(&fd_table[i], 0, sizeof(fd_t));
            fd_table[i].used = 1;
            return i;
        }
    }
    return -1;
}

int pipe_alloc(int *fd) {
    int i, fd_0 = 0;
    for (i = STDMAX; i < MAX_FD; i++) {
        if (!fd_table[i].used) {
            fd_table[i].used = 1;
            fd_table[i].pip_num = pipe_cnt;
            fd_table[i].is_pipe_read = TRUE;
            fd_table[i].is_pipe_write = FALSE;
            fd[0] = i;
            fd_0 = 1;
            break;
        }
    }
    if (!fd_0) {
        return -1;
    }
    for (i = STDMAX; i < MAX_FD; i++) {
        if (!fd_table[i].used) {
            fd_table[i].used = 1;
            fd_table[i].pip_num = pipe_cnt++;
            fd_table[i].is_pipe_read = FALSE;
            fd_table[i].is_pipe_write = TRUE;
            fd[1] = i;
            return 0;
        }
    }
    if (fd_0) {
        fd_free(fd[0]);
    }
    return -1;
}

fd_t *fd_alloc_spec(int fd) {
    if (fd_table[fd].used) {
        return NULL;
    }
    fd_table[fd].used = 1;
    return &fd_table[fd];
}

int fd_free(int fd) {
    int ret = 0;
    if (fd < STDMAX || fd >= MAX_FD) {
        return -1;
    }
    if (fd_table[fd].used == 0) {
        ret = -1;
    }
    fd_table[fd].used = 0;
    return ret;
}

fd_t *get_fd(int fd) {
    if (fd < STDIN || fd >= MAX_FD) {
        return NULL;
    }
    return &fd_table[fd];
}

void ring_buffer_init(struct ring_buffer *rbuf) {
    // there is always one byte which should not be read or written
    k_memset(rbuf, 0, sizeof(struct ring_buffer)); /* head = tail = 0 */
    rbuf->size = RING_BUFFER_SIZE;
    rbuf->buf = k_mm_malloc(PAGE_SIZE);
    k_memset(rbuf->buf, 0, PAGE_SIZE);
    // TODO INIT LOCK
    k_spin_lock_init(&rbuf->lock);
    return;
}

size_t read_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size) {
    // TODO pcb and lock
    k_spin_lock_acquire(&rbuf->lock);
    int32_t len = min(ring_buffer_used(rbuf), size);
    if (len > 0) {
        if (rbuf->head + len > rbuf->size) {
            int32_t right = rbuf->size - rbuf->head, left = len - right;
            k_memcpy(buf, rbuf->buf + rbuf->head, right);
            k_memcpy(buf + right, rbuf->buf, left);
        } else {
            k_memcpy(buf, rbuf->buf + rbuf->head, len);
        }

        rbuf->head = (rbuf->head + len) % (rbuf->size);
    }
    k_spin_lock_release(&rbuf->lock);
    return len;
}
size_t write_ring_buffer(struct ring_buffer *rbuf, uint8_t *buf, size_t size) {
    // TODO SYNC BY LOCK
    k_spin_lock_acquire(&rbuf->lock);
    int32_t len = min(ring_buffer_free(rbuf), size);
    if (len > 0) {
        if (rbuf->tail + len > rbuf->size) {
            int32_t right = rbuf->size - rbuf->tail, left = len - right;
            k_memcpy(rbuf->buf + rbuf->tail, buf, right);
            if (left > 0) {
                k_memcpy(rbuf->buf, buf + right, left);
            }
        } else {
            k_memcpy(rbuf->buf + rbuf->tail, buf, len);
        }

        rbuf->tail = (rbuf->tail + len) % (rbuf->size);
    }
    k_spin_lock_release(&rbuf->lock);
    return len;
}