#include "hook.h"
#include "fiber.h"
#include "iomanager.h"

#include <dlfcn.h>
#include <sys/types.h>

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

#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX)
#undef XX
}

struct _HookIniter {
  _HookIniter() { hook_init(); }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }
void set_hook_enable(bool flag) { t_hook_enable = flag; }

} // namespace apexstorm

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX)
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!apexstorm::t_hook_enable) {
    return sleep_f(seconds);
  }

  apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
  iom->addTimer(seconds * 1000,
                std::bind([iom, fiber]() { iom->schedule(fiber); }));
  apexstorm::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!apexstorm::t_hook_enable) {
    return usleep_f(usec);
  }

  apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
  iom->addTimer(usec / 1000,
                std::bind([iom, fiber]() { iom->schedule(fiber); }));
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
  iom->addTimer(timeout_ms,
                std::bind([iom, fiber]() { iom->schedule(fiber); }));
  apexstorm::Fiber::YieldToHold();
  return 0;
}
}
