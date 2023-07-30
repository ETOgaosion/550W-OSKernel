#include <asm/sbi.h>
#include <os/socket.h>

// [TODO]
long sys_socket(int domain, int type, int protocol) {
    return 0;
}

long sys_socketpair(int domain, int type, int protocol, void *sv) {
    return 0;
}

long sys_bind(int sockfd, sockaddr_t *addr, int addrlen) {
    return 0;
}

long sys_listen(int sockfd, int backlog) {
    return 0;
}

long sys_accept(int sockfd, sockaddr_t *addr, int *addrlen) {
    return 0;
}

long sys_connect(int sockfd, sockaddr_t *addr, int addrlen) {
    return 0;
}

long sys_getsockname(int sockfd, sockaddr_t *addr, int *addrlen) {
    return 0;
}

long sys_getpeername(int sockfd, sockaddr_t *addr, int *addrlen) {
    return 0;
}

long sys_sendto(int sockfd, void *message, size_t length, unsigned flags, sockaddr_t *dest_addr, int dest_len) {
    return 0;
}

long sys_recvfrom(int sockfd, void *buffer, size_t length, unsigned flags, sockaddr_t *address, int *address_len) {
    return 0;
}

long sys_setsockopt(int fd, int level, int optname, char *optval, int optlen) {
    return 0;
}

long sys_getsockopt(int fd, int level, int optname, char *optval, int *optlen) {
    return 0;
}

long sys_shutdown(int sockfd, int how) {
    sbi_shutdown();
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