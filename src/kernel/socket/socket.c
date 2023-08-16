#include <asm/sbi.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/pcb.h>
#include <os/socket.h>
#include <fs/fs.h>
#include <fs/file.h>

socket_t sock[MAX_SOCK_NUM + 1];
int sock_lock;

void k_socket_init() {
    k_mutex_lock_init(&sock_lock);
    for (int i = 0; i <= MAX_SOCK_NUM; i++) {
        k_bzero((void *)&sock[i], sizeof(socket_t));
        k_ring_buffer_init(&sock[i].data);
    }
}

// [TODO]
long sys_socket(int domain, int type, int protocol) {
    int socket_i = 0;
    k_mutex_lock_acquire(sock_lock);
    for (int i = 1; i <= MAX_SOCK_NUM; i++) {
        if (!sock[i].used) {
            socket_i = i;
            break;
        }
    }
    if (socket_i == 0) {
        k_mutex_lock_release(sock_lock);
        return -ENFILE;
    }
    sock[socket_i].used = 1;
    sock[socket_i].domain = domain;
    sock[socket_i].status = SOCK_CLOSED;
    sock[socket_i].type = type;
    sock[socket_i].protocol = protocol;
    int new_fd_num = fd_alloc(-1);
    if (new_fd_num == -1) return -ENFILE;
    fd_t *file = get_fd(new_fd_num);
    file->dev = DEV_SOCK;
    file->sock_num = socket_i;
    if (type & SOCK_CLOEXEC) file->mode |= FD_CLOEXEC;
    if (type & SOCK_NONBLOCK) file->flags |= O_NONBLOCK;
    k_mutex_lock_release(sock_lock);
    // return new_fd_num;
    return 0;
}

long sys_socketpair(int domain, int type, int protocol, void *sv) {
    int socket_i = 0, socket_j = 0;
    k_mutex_lock_acquire(sock_lock);
    for (int i = 1; i <= MAX_SOCK_NUM; i++) {
        if (!sock[i].used) {
            if (socket_i == 0) {
                socket_i = i;
            } else {
                socket_j = i;
                break;
            }
        }
    }
    if (socket_i == 0 || socket_j == 0) {
        k_mutex_lock_release(sock_lock);
        return -ENFILE;
    }
    sock[socket_i].used = 1;
    sock[socket_i].domain = domain;
    sock[socket_i].status = SOCK_CLOSED;
    sock[socket_i].type = type;
    sock[socket_i].protocol = protocol;

    sock[socket_j].used = 1;
    sock[socket_j].domain = domain;
    sock[socket_j].status = SOCK_CLOSED;
    sock[socket_j].type = type;
    sock[socket_j].protocol = protocol;

    int new_fd_num = fd_alloc(-1);
    if (new_fd_num == -1) return -ENFILE;
    fd_t *file = get_fd(new_fd_num);
    file->dev = DEV_SOCK;
    file->sock_num = socket_i;
    if (type & SOCK_CLOEXEC) file->mode |= FD_CLOEXEC;
    if (type & SOCK_NONBLOCK) file->flags |= O_NONBLOCK;
    ((int *)sv)[0] = new_fd_num;

    new_fd_num = fd_alloc(-1);
    if (new_fd_num == -1) return -ENFILE;
    file = get_fd(new_fd_num);
    file->dev = DEV_SOCK;
    file->sock_num = socket_j;
    if (type & SOCK_CLOEXEC) file->mode |= FD_CLOEXEC;
    if (type & SOCK_NONBLOCK) file->flags |= O_NONBLOCK;
    ((int *)sv)[1] = new_fd_num;
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_bind(int sockfd, sockaddr_t *addr, int addrlen) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    k_memcpy(&sock[sock_num].addr, addr, sizeof(sockaddr_t));
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_listen(int sockfd, int backlog) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    sock[sock_num].status = SOCK_LISTEN;
    sock[sock_num].backlog = backlog;
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_accept(int sockfd, sockaddr_t *addr, int *addrlen) {
    assert(*addrlen == sizeof(sockaddr_t));
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    int i;
    k_mutex_lock_acquire(sock_lock);
    for (i = 0; i < sock[sock_num].backlog; i++) {
        if (sock[sock_num].wait_list[i]) {
            break;
        }
    }
    if (i == sock[sock_num].backlog) {
        k_mutex_lock_release(sock_lock);
        return -EINVAL;
    }
    sock[sock[sock_num].wait_list[i]].status = SOCK_ACCEPTED;
    sock[sock_num].wait_list[i] = 0;
    k_mutex_lock_release(sock_lock);
    int new_sock = sys_socket(sock[sock_num].domain, sock[sock_num].type, sock[sock_num].protocol);
    k_mutex_lock_acquire(sock_lock);
    fd_t *new_file = get_fd(new_sock);
    sock[new_file->sock_num].status = SOCK_ESTABLISHED;
    k_memcpy(&sock[new_file->sock_num].remote_addr, addr, sizeof(sockaddr_t));
    k_mutex_lock_release(sock_lock);
    return new_sock;
}

long sys_connect(int sockfd, sockaddr_t *addr, int addrlen) {
    if (addrlen != sizeof(sockaddr_t)) {
        return -EINVAL;
    }
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    int i, j;
    k_mutex_lock_acquire(sock_lock);
    for (i = 1; i <= MAX_SOCK_NUM; i++) {
        if (!k_memcmp(addr, &sock[i].addr, sizeof(sockaddr_t)) && sock[i].status == SOCK_LISTEN) {
            k_memcpy(&sock[i].remote_addr, addr, sizeof(sockaddr_t));
            break;
        }
    }
    if (i == MAX_SOCK_NUM + 1) {
        k_mutex_lock_release(sock_lock);
        return -EINVAL;
    }
    for (j = 0; j < sock[i].backlog; j++) {
        if (!sock[i].wait_list[j]) {
            sock[i].wait_list[j] = sock_num;
            break;
        }
    }
    if (j == sock[i].backlog) {
        k_mutex_lock_release(sock_lock);
        return -EINVAL;
    }
    if (sock[sock_num].type & SOCK_NONBLOCK) {
        sock[sock_num].status = SOCK_ACCEPTED;
    } else {
        while (sock[sock_num].status != SOCK_ACCEPTED) {
            k_mutex_lock_release(sock_lock);
            k_pcb_scheduler(false, false);
            k_mutex_lock_acquire(sock_lock);
        }
    }
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_getsockname(int sockfd, sockaddr_t *addr, int *addrlen) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    k_memcpy(addr, &sock[sock_num].addr, sizeof(sockaddr_t));
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_getpeername(int sockfd, sockaddr_t *addr, int *addrlen) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    k_memcpy(addr, &sock[sock_num].remote_addr, sizeof(sockaddr_t));
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_sendto(int sockfd, void *message, size_t length, unsigned flags, sockaddr_t *dest_addr, int dest_len) {
    int i = 0;
    k_mutex_lock_acquire(sock_lock);
    for (i = 1; i <= MAX_SOCK_NUM; i++) {
        if (!k_memcmp(dest_addr, &sock[i].addr, sizeof(sockaddr_t)) && ((sock[i].protocol == IPPROTO_UDP) || (sock[i].protocol == IPPROTO_TCP && sock[i].status == SOCK_ESTABLISHED))) {
            break;
        }
    }
    if (i == MAX_SOCK_NUM + 1) {
        k_mutex_lock_release(sock_lock);
        return -EINVAL;
    }
    int old_len = length;
    while (length > 0) {
        int write_len = k_ring_buffer_write(&sock[i].data, message, length);
        message += write_len;
        length -= write_len;
    }
    k_mutex_lock_release(sock_lock);
    return old_len;
}

long sys_recvfrom(int sockfd, void *buffer, size_t length, unsigned flags, sockaddr_t *address, int *address_len) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    int read_len = (length < k_ring_buffer_used(&sock[sock_num].data)) ? length : k_ring_buffer_used(&sock[sock_num].data);
    k_ring_buffer_read(&sock[sock_num].data, buffer, read_len);
    k_mutex_lock_release(sock_lock);
    return read_len;
}

long sys_setsockopt(int fd, int level, int optname, char *optval, int optlen) {
    return 0;
}

long sys_getsockopt(int fd, int level, int optname, char *optval, int *optlen) {
    return 0;
}

long sys_shutdown(int sockfd, int how) {
    int sock_num = 0;
    fd_t *file = get_fd(sockfd);
    if(!file)
        return -ENFILE;
    sock_num = file->sock_num;
    k_mutex_lock_acquire(sock_lock);
    k_memset(&sock[sock_num], 0, sizeof(struct socket));
    k_ring_buffer_init(&sock[sock_num].data);
    k_mutex_lock_release(sock_lock);
    return 0;
}

long sys_sendmsg(int fd, user_msghdr_t *msg, unsigned flags) {
    return 0;
}

long sys_recvmsg(int fd, user_msghdr_t *msg, unsigned flags) {
    return 0;
}

long sys_readahead(int fd, loff_t offset, size_t count) {
    return 0;
}