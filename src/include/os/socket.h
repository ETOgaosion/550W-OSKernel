#pragma once

#include <common/types.h>
#include <fs/fs.h>

typedef unsigned short __kernel_sa_family_t;
typedef __kernel_sa_family_t sa_family_t;

typedef struct sockaddr_t {
    sa_family_t sa_family; /* address family, AF_xxx	*/
    char sa_data[];        /* Socket address */
} sockaddr_t;

typedef struct user_msghdr {
    void *msg_name;                 /* ptr to socket address structure */
    int msg_namelen;                /* size of socket address structure */
    iovec_t *msg_iov;               /* scatter/gather array */
    __kernel_size_t msg_iovlen;     /* # elements in msg_iov */
    void *msg_control;              /* ancillary data */
    __kernel_size_t msg_controllen; /* ancillary data buffer length */
    unsigned int msg_flags;         /* flags on received message */
} user_msghdr_t;

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