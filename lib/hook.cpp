#include "hook.h"
#include "config.h"
#include "fdmanager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"
#include "util.h"
#include "yaml-cpp/emittermanip.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include <iterator>
#include <memory>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

static apexstorm::ConfigVar<int>::ptr g_tcp_connect_timeout =
    apexstorm::Config::Lookup("tcp.connect.timeout", 5000,
                              "tcp connect timeout");

namespace apexstorm {

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)                                                           \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(nanosleep)                                                                \
  XX(socket)                                                                   \
  XX(connect)                                                                  \
  XX(accept)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)                                                                    \
  XX(fcntl)                                                                    \
  XX(ioctl)                                                                    \
  XX(getsockopt)                                                               \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }

  // such as: name = sleep
  // sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
  // [sleep_fun]: pointer to sleep.
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX) // relevant hook api
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();

    s_connect_timeout = g_tcp_connect_timeout->getValue();
    g_tcp_connect_timeout->addListener(
        [](const int &old_value, const int &new_value) {
          APEXSTORM_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                       << old_value << " to " << new_value;
          s_connect_timeout = new_value;
        });
  }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }
void set_hook_enable(bool flag) { t_hook_enable = flag; }

struct timer_info {
  int cancelled = 0;
};

// template function for relevant doing io operations.
// @param: fd - file descriptor
// @param: fun - the original hook function
// @param: hook_fun_name - the original hook function name
// @param: event - IOManager::Event(WRITE/READ)
// @param: timeout_so - SO_RCVTIMEO(read timeout) / SO_SNDTIMEO(write timeout)
// @param: args - paramater pack (using std::forward<Args> to perfecting
// forwarding)
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
  if (!apexstorm::t_hook_enable) {
    // if not enabling hooking, call original function and parsing parameter
    // pack
    return fun(fd, std::forward<Args>(args)...);
  }

  // acquire the file descriptor corresponding context
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    // this fd may not register in FdManager, maybe created by non-hooking way.
    return fun(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClosed()) {
    // Bad file number
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    // no socket or user dont't wanted to non-blocking
    return fun(fd, std::forward<Args>(args)...);
  }

  // acquire corrsponding (read/write) timeout from fd context
  uint64_t timeout = ctx->getTimeout(timeout_so);
  // create a delaying resource
  std::shared_ptr<timer_info> tinfo(new timer_info);

// goto label, in order to get data
retry:
  // n indicates the amount bytes of data already read from or written into.
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  // handler errno code, Interrupted system call
  while (n == -1 && errno == EINTR) {
    // until the interrupted end up.
    n = fun(fd, std::forward<Args>(args)...);
  }
  // handler errno code, Try again
  // 1. the io buffer has not data when reading, or io buffer is full when
  // writing
  // 2. the resource was locked.
  // 3. fcntl / ioctl operation cannot be completed immediately or for a short
  // period of time.
  if (n == -1 && errno == EAGAIN) {
    apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
    apexstorm::Timer::ptr timer;
    // condition variable weak reference
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout != (uint64_t)-1) {
      // having timeout value

      // add condition delaying timer
      timer = iom->addConditionTimer(
          timeout,
          [winfo, fd, iom, event]() {
            // condition variable is not existed
            auto t = winfo.lock();
            // or being cancelled
            if (!t || t->cancelled) {
              return;
            }
            // set Connection timed out
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (apexstorm::IOManager::Event)(event));
          },
          winfo);
    }

    // add relevant listening event about this fd.
    int rt = iom->addEvent(fd, (apexstorm::IOManager::Event)(event));
    if (rt) {
      // adding failure
      APEXSTORM_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      // adding success, yield out current fiber
      APEXSTORM_LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
      apexstorm::Fiber::YieldToHold();
      APEXSTORM_LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
      // yield come back condition:
      // 1.timer expired
      // 2.data comes
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      // [mentioned]: data initialized?
      goto retry;
    }
  }
  return n;
}

} // namespace apexstorm

extern "C" {

// name = sleep
// sleep_fun sleep_f = nullptr;
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX)
#undef XX

// sleep

unsigned int sleep(unsigned int seconds) {
  if (!apexstorm::t_hook_enable) {
    return sleep_f(seconds);
  }

  apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();

  // adapting template function: `do_io`
  // transformation:
  // void apexstorm::Scheduler::schedule(cb, int) ->
  // (void(apexstorm::Scheduler::*)(apexstorm::Fiber::ptr, int thread))

  // iom->addTimer(seconds * 1000,
  //               std::bind([iom, fiber]() { iom->schedule(fiber); }));
  iom->addTimer(seconds * 1000,
                std::bind((void(apexstorm::Scheduler::*)(apexstorm::Fiber::ptr,
                                                         int thread)) &
                              apexstorm::IOManager::schedule,
                          iom, fiber, -1));

  apexstorm::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!apexstorm::t_hook_enable) {
    return usleep_f(usec);
  }

  apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();

  // iom->addTimer(usec / 1000,
  //               std::bind([iom, fiber]() { iom->schedule(fiber); }));
  iom->addTimer(usec / 1000, std::bind((void(apexstorm::Scheduler::*)(
                                           apexstorm::Fiber::ptr, int thread)) &
                                           apexstorm::Scheduler::schedule,
                                       iom, fiber, -1));

  apexstorm::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!apexstorm::t_hook_enable) {
    return nanosleep(req, rem);
  }
  apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  // iom->addTimer(timeout_ms,
  //               std::bind([iom, fiber]() { iom->schedule(fiber); }));
  iom->addTimer(timeout_ms, std::bind((void(apexstorm::Scheduler::*)(
                                          apexstorm::Fiber::ptr, int thread)) &
                                          apexstorm::IOManager::schedule,
                                      iom, fiber, -1));
  apexstorm::Fiber::YieldToHold();
  return 0;
}

// socket

int socket(int domain, int type, int protocol) {
  if (!apexstorm::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  apexstorm::FdMgr::GetInstance()->get(fd, true);
  return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  if (!apexstorm::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClosed()) {
    // Bad file number
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket()) {
    // no socket type
    return connect_f(fd, addr, addrlen);
  }

  // Get whether user intervention to set non-blocking.
  if (ctx->getUserNonblock()) {
    return connect_f(fd, addr, addrlen);
  }

  // blocking connect
  int n = connect_f(fd, addr, addrlen);

  if (n == 0) {
    return 0;
  } else if (n != -1 || errno != EINPROGRESS) {
    // Operation now not in progress same as `do_io`
    return n;
  }

  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
  apexstorm::Timer::ptr timer;

  std::shared_ptr<apexstorm::timer_info> tinfo(new apexstorm::timer_info);
  std::weak_ptr<apexstorm::timer_info> winfo(tinfo);

  if (timeout_ms != (uint64_t)-1) {
    // have set timeout
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, fd, iom]() {
          auto t = winfo.lock();
          if (!t || t->cancelled) {
            return;
          }
          t->cancelled = ETIMEDOUT;
          iom->cancelEvent(fd, apexstorm::IOManager::Event::WRITE);
        },
        winfo);
  }

  int rt = iom->addEvent(fd, apexstorm::IOManager::Event::WRITE);
  if (rt == 0) {
    apexstorm::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer) {
      timer->cancel();
    }
    APEXSTORM_LOG_ERROR(g_logger)
        << "connect addEvent(" << fd << ", WRITE) error";
  }

  // check socket status
  int error = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  if (!error) {
    return 0;
  } else {
    errno = error;
    return -1;
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  // return connect_f(sockfd, addr, addrlen);
  return connect_with_timeout(sockfd, addr, addrlen,
                              apexstorm::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = apexstorm::do_io(sockfd, accept_f, "accept",
                            (uint32_t)apexstorm::IOManager::Event::READ,
                            SO_RCVTIMEO, addr, addrlen);
  if (fd >= 0) {
    apexstorm::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}

// socket read

ssize_t read(int fd, void *buf, size_t count) {
  return apexstorm::do_io(fd, read_f, "read",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return apexstorm::do_io(fd, readv_f, "readv",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return apexstorm::do_io(sockfd, recv_f, "recv",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return apexstorm::do_io(sockfd, recvfrom_f, "recvfrom",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return apexstorm::do_io(sockfd, recvmsg_f, "recvmsg",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, msg, flags);
}

// socket write

ssize_t write(int fd, const void *buf, size_t count) {
  return apexstorm::do_io(fd, write_f, "write",
                          (uint32_t)apexstorm::IOManager::Event::WRITE,
                          SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return apexstorm::do_io(fd, writev_f, "writev",
                          (uint32_t)apexstorm::IOManager::Event::WRITE,
                          SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  return apexstorm::do_io(sockfd, send_f, "send",
                          (uint32_t)apexstorm::IOManager::Event::WRITE,
                          SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  return apexstorm::do_io(sockfd, sendto_f, "sendto",
                          (uint32_t)apexstorm::IOManager::Event::WRITE,
                          SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return apexstorm::do_io(sockfd, sendmsg_f, "sendmsg",
                          (uint32_t)apexstorm::IOManager::Event::WRITE,
                          SO_SNDTIMEO, msg, flags);
}

// socket close
int close(int fd) {
  if (!apexstorm::t_hook_enable) {
    return close_f(fd);
  }

  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (ctx) {
    auto iom = apexstorm::IOManager::GetThis();
    if (iom) {
      iom->cancelAll(fd);
    }
    apexstorm::FdMgr::GetInstance()->del(fd);
  }
  return close_f(fd);
}

// fcntl
int fcntl(int fd, int cmd, ... /* arg */) {
  va_list va;
  va_start(va, cmd);
  // hook towards `F_SETFL` and `F_GETFL`
  switch (cmd) {
  case F_SETFL: {
    int arg = va_arg(va, int);
    va_end(va);
    // params: int
    apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      // this fd context doesn't register to the fd manager
      // or this fd is closed
      // or this fd is not a socket fd
      return fcntl_f(fd, cmd, arg);
    }
    // set user non-blocking flags to this socket fd
    ctx->setUserNonblock(arg & O_NONBLOCK);
    // check whether is system wanted to non-blocking
    if (ctx->getSysNonblock()) {
      arg |= O_NONBLOCK;
    } else {
      arg &= ~O_NONBLOCK;
    }
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETFL: {
    va_end(va);
    // param: empty
    int arg = fcntl_f(fd, cmd);
    apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      // this fd context doesn't register to the fd manager
      // or this fd is closed
      // or this fd is not a socket fd
      return arg;
    }
    // whether user wanted to non-blocking
    if (ctx->getUserNonblock()) {
      return arg | O_NONBLOCK;
    } else {
      return arg & ~O_NONBLOCK;
    }
  } break;

  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  case F_SETFD:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
  case F_SETPIPE_SZ: {
    int arg = va_arg(va, int);
    va_end(va);
    int rt = fcntl_f(fd, cmd, arg);
    return rt;
  } break;
  case F_GETFD:
  case F_GETOWN:
  case F_GETSIG:
  case F_GETLEASE:
  case F_GETPIPE_SZ: {
    return fcntl(fd, cmd);
  } break;
  case F_SETLK:
  case F_SETLKW:
  case F_GETLK: {
    struct flock *arg = va_arg(va, struct flock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETOWN_EX:
  case F_SETOWN_EX: {
    struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  default: {
    return fcntl_f(fd, cmd);
  } break;
  }
}

// ioctl
int ioctl(int fd, unsigned long request, ...) {
  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);
  // param: void*

  // requesting non-blocking
  if (FIONBIO == request) {
    // a integer pointer
    bool user_nonblock = !!*(int *)arg;
    // non-blocking = 1
    apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      // this fd context doesn't register to the fd manager
      // or this fd is closed
      // or this fd is not a socket fd
      return ioctl_f(fd, request, arg);
    }
    ctx->setUserNonblock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

// socket get/set options

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (!apexstorm::t_hook_enable) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(sockfd);
      if (ctx) {
        // convert optval -> timeout structure
        const timeval *v = (const timeval *)optval;
        ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
