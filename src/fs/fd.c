#include <fs/file.h>
#include <fs/fs.h>
// #include <os/time.h>
#include <os/pcb.h>
// #include <fs/pipe.h>

#include <lib/stdio.h>
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

int get_fd_from_list(){
    pcb_t *pcb = *current_running;
    if(list_is_empty(&pcb->fd_head)){
        return STDMAX;
    }
    int fd = STDMAX;
    fd_t *file;
    list_for_each_entry(file,&pcb->fd_head,list){
        if(file->fd_num == fd)
            fd++;
        else
            return fd;
    }
    return fd;
}

//test if fd exist
fd_t *fd_exist(int fd){
    pcb_t *pcb = *current_running;
    if(list_is_empty(&pcb->fd_head)){
        return 0;
    }
    fd_t *file;
    list_for_each_entry(file,&pcb->fd_head,list){
        if(file->fd_num == fd){
            return file;
        }
        if(file->fd_num > fd)
            return NULL;
    }
    return NULL;
}

//fd = -1, get fd, or spec fd
// -1 fail, or return fd
int fd_alloc(int fd) {
    // int i;
    // for (i = STDMAX; i < MAX_FD; i++) {
    //     if (!fd_table[i].used) {
    //         k_bzero(&fd_table[i], sizeof(fd_t));
    //         fd_table[i].used = 1;
    //         return i;
    //     }
    // }
    // return -1;
    //访问PCB
    pcb_t *pcb = *current_running;
    k_print("[debug] pcb id is %d\n",pcb->pid);
    //get fd num
    if(fd == -1)
        fd = get_fd_from_list();
    else{
        if(fd_exist(fd)){
            return -1;
        }
    }
    k_print("[debug] get fd %d\n",fd);
    //alloc fd
    fd_t *file = (fd_t *)k_mm_malloc(sizeof(fd_t));
    k_memset(file,0,sizeof(fd_t));
    file->used = 1;
    file->fd_num = fd;
    //add to fd list in pcb
    fd_t *pos;
    list_for_each_entry(pos,&pcb->fd_head,list){
        if(pos->fd_num > fd){
            __list_add(&file->list,pos->list.prev,&pos->list);
            return fd;
        }
    }
    __list_add(&file->list,pcb->fd_head.prev,&pcb->fd_head);
    return fd;
}

int pipe_alloc(int *fd) {
    // int i, fd_0 = 0;
    // for (i = STDMAX; i < MAX_FD; i++) {
    //     if (!fd_table[i].used) {
    //         fd_table[i].used = 1;
    //         fd_table[i].pip_num = pipe_cnt;
    //         fd_table[i].is_pipe_read = TRUE;
    //         fd_table[i].is_pipe_write = FALSE;
    //         fd[0] = i;
    //         fd_0 = 1;
    //         break;
    //     }
    // }
    // if (!fd_0) {
    //     return -1;
    // }
    // for (i = STDMAX; i < MAX_FD; i++) {
    //     if (!fd_table[i].used) {
    //         fd_table[i].used = 1;
    //         fd_table[i].pip_num = pipe_cnt++;
    //         fd_table[i].is_pipe_read = FALSE;
    //         fd_table[i].is_pipe_write = TRUE;
    //         fd[1] = i;
    //         return 0;
    //     }
    // }
    // if (fd_0) {
    //     fd_free(fd[0]);
    // }
    fd[0] = fd_alloc(-1);
    if(fd[0] == -1)
        return -1;
    fd[1] = fd_alloc(-1);
    if(fd[1] == -1){
        fd_free(fd[0]);
        return -1;
    }
    fd_t *file1,*file2;
    file1 = fd_exist(fd[0]);
    file1->used = 1;
    file1->pip_num = pipe_cnt;
    file1->is_pipe_read = TRUE;
    file1->is_pipe_write = FALSE;
    
    file2 = fd_exist(fd[1]);
    file2->used = 1;
    file2->pip_num = pipe_cnt++;
    file2->is_pipe_read = FALSE;
    file2->is_pipe_write = TRUE;
    k_print("[debug] alloc pipe %d %d\n",fd[0],fd[1]);

    return 0;
}

int fd_free(int fd) {
    int ret = 0;
    if (fd < STDMAX || fd >= MAX_FD) {
        return -1;
    }
    fd_t *file = fd_exist(fd);
    if(!file)
        return -1;
    __list_del(file->list.prev,file->list.next);
    //TODO free
    //k_free(file);
    return ret;
}

fd_t *get_fd(int fd) {
    if (fd < STDMAX || fd >= MAX_FD) {
        return NULL;
    }
    return fd_exist(fd);
}

void ring_buffer_init(struct ring_buffer *rbuf) {
    // there is always one byte which should not be read or written
    k_bzero(rbuf, sizeof(struct ring_buffer)); /* head = tail = 0 */
    rbuf->size = RING_BUFFER_SIZE;
    rbuf->buf = k_mm_malloc(PAGE_SIZE);
    k_bzero(rbuf->buf, PAGE_SIZE);
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