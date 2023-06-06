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
            fd_table[i].is_pipe_read = true;
            fd[0] = i;
            fd_0 = 1;
        }
    }
    if (!fd_0) {
        return -1;
    }
    for (i = STDMAX; i < MAX_FD; i++) {
        if (!fd_table[i].used) {
            fd_table[i].used = 1;
            fd_table[i].pip_num = pipe_cnt++;
            fd_table[i].is_pipe_write = true;
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
