#ifndef __APEXSTORM_HOOK_H__
#define __APEXSTORM_HOOK_H__

#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

namespace apexstorm {

bool is_hook_enable();
void set_hook_enable(bool flag);

} // namespace apexstorm

extern "C" {
// sleep hook

typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

// socket hook

typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int socketfd, const struct sockaddr *addr,
                           socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int socketfd, const struct sockaddr *addr,
                          socklen_t addrlen);
extern accept_fun accept_f;

// socket read hook

typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr src_addr, socklen_t addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

// socket write hook
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len,
                              int flags, const struct sockaddr *dest_addr,
                              socklen_t addrlen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

// socket close
typedef int (*close_fun)(int fd);
extern close_fun close_f;

// fcntl
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
extern fcntl_fun fcntl_f;

// ioctl
typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
extern ioctl_fun ioctl_f;

// socket get/set options

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval,
                              socklen_t optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;
}

#endif