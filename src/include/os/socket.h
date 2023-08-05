#pragma once

#include <common/types.h>
#include <fs/fs.h>
#include <lib/ring_buffer.h>

#define PF_INET 2
#define AF_INET PF_INET

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define SOL_SOCKET 1

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

#define SOCK_CLOEXEC 02000000
#define SOCK_NONBLOCK 04000
#define FD_CLOEXEC 1
#define O_NONBLOCK (SOCK_NONBLOCK)
#define SO_RCVTIMEO 20

#define MAX_SOCK_NUM 16
#define MAX_WAIT_LIST 8

#define SOCK_CLOSED 0
#define SOCK_LISTEN 1
#define SOCK_ACCEPTED 2
#define SOCK_ESTABLISHED 3

typedef unsigned short kernel_sa_family_t;
typedef kernel_sa_family_t sa_family_t;

struct in_addr {
    in_addr_t s_addr;
};

typedef struct sockaddr {
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    uint8_t sin_zero[8];
} sockaddr_t;

typedef struct socket {
    int domain;
    int type;
    int protocol;
    int backlog;
    uint8_t wait_list[MAX_WAIT_LIST];
    uint8_t used;
    uint8_t status;
    sockaddr_t addr;
    sockaddr_t remote_addr;
    ring_buffer_t data;
} socket_t;

typedef struct user_msghdr {
    void *msg_name;               /* ptr to socket address structure */
    int msg_namelen;              /* size of socket address structure */
    iovec_t *msg_iov;             /* scatter/gather array */
    kernel_size_t msg_iovlen;     /* # elements in msg_iov */
    void *msg_control;            /* ancillary data */
    kernel_size_t msg_controllen; /* ancillary data buffer length */
    unsigned int msg_flags;       /* flags on received message */
} user_msghdr_t;

extern socket_t sock[MAX_SOCK_NUM + 1];
extern int sock_lock;

void k_socket_init();

long sys_socket(int domain, int type, int protocol);

long sys_socketpair(int domain, int type, int protocol, void *sv);

long sys_bind(int sockfd, sockaddr_t *addr, int addrlen);

long sys_listen(int sockfd, int backlog);

long sys_accept(int sockfd, sockaddr_t *addr, int *addrlen);

long sys_connect(int sockfd, sockaddr_t *addr, int addrlen);

long sys_getsockname(int sockfd, sockaddr_t *addr, int *addrlen);

long sys_getpeername(int sockfd, sockaddr_t *addr, int *addrlen);

long sys_sendto(int sockfd, void *message, size_t length, unsigned flags, sockaddr_t *dest_addr, int dest_len);

long sys_recvfrom(int sockfd, void *buffer, size_t length, unsigned flags, sockaddr_t *address, int *address_len);

long sys_setsockopt(int fd, int level, int optname, char *optval, int optlen);

long sys_getsockopt(int fd, int level, int optname, char *optval, int *optlen);

long sys_shutdown(int sockfd, int how);

long sys_sendmsg(int fd, user_msghdr_t *msg, unsigned flags);

long sys_recvmsg(int fd, user_msghdr_t *msg, unsigned flags);

long sys_readahead(int fd, loff_t offset, size_t count);