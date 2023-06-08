#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <type_traits>

namespace apexstorm {

// -- EPOLL_EVENTS(Event) stringstream <<
std::ostream &operator<<(std::ostream &os, IOManager::Event e) {
  return os << EPOLL_EVENTS(e);
}
std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events) {
  if (!events) {
    return os << "None";
  }
  bool first = true;
#define XX(E)                                                                  \
  if (events & E) {                                                            \
    if (!first) {                                                              \
      os << "|";                                                               \
    }                                                                          \
    os << #E;                                                                  \
    first = false;                                                             \
  }
  // --- --- --- --- --- --- --- --EPOLL EVENT-- --- --- --- --- --- --- --- ---
  // EPOLLIN: indicates that there is data to be read from a file descriptor,
  // or a socket has become readable.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLPRI: indicates that there is urgent data to be read on a socket.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLOUT: indicates that a file descriptor or socket is ready for writing.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLRDNORM: indicates that there is data to be read from a file descriptor
  // or socket.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLRDBAND: indicates that there is band-priority data to be read from a
  // socket.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLWRNORM: indicates that a file descriptor or socket can be written to.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLWRBAND: indicates that there is band-priority data to be written to a
  // socket.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLMSG: indicates that a socket has received a message.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLERR: indicates that an error has occurred on a file descriptor or
  // socket.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLHUP: indicates that a file descriptor or socket has been closed, or
  // the remote end of a socket has shut down the connection.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLRDHUP: indicates that the peer has shut down its half of the
  // connection.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLONESHOT: indicates that the file descriptor or socket should only
  // be monitored for a single event, after which it will be removed from the
  // epoll set.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  // EPOLLET: indicates that epoll should operate in edge-triggered mode,
  // where the application will only be notified of events once per transition
  // rather than continuously while the event is pending.
  // --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
  XX(EPOLLIN);
  XX(EPOLLPRI);
  XX(EPOLLOUT);
  XX(EPOLLRDNORM);
  XX(EPOLLRDBAND);
  XX(EPOLLWRNORM);
  XX(EPOLLWRBAND);
  XX(EPOLLMSG);
  XX(EPOLLERR);
  XX(EPOLLHUP);
  XX(EPOLLRDHUP);
  XX(EPOLLONESHOT);
  XX(EPOLLET);
#undef XX
  return os;
}
// -- EPOLL_EVENTS | Event
IOManager::Event operator|(EPOLL_EVENTS l, IOManager::Event r) {
  return IOManager::Event(l | (uint32_t)r);
}
// -- Event & Event
IOManager::Event operator&(const IOManager::Event l, const IOManager::Event r) {
  return IOManager::Event(static_cast<uint32_t>(l) & static_cast<uint32_t>(r));
}

// int operator&(const IOManager::Event &l,
//               const IOManager::Event &r) { // TODO: test reference
//   return static_cast<int>(l) & static_cast<int>(r);
// }
bool operator&(const IOManager::Event e, int flg) {
  return static_cast<uint32_t>(e) & flg;
}
// -- Event |
IOManager::Event operator|(const IOManager::Event &l,
                           const IOManager::Event &r) {
  return (IOManager::Event)(static_cast<uint32_t>(l) |
                            static_cast<uint32_t>(r));
}
IOManager::Event operator|(const IOManager::Event &e, uint32_t flg) {
  return (IOManager::Event)(static_cast<uint32_t>(e) | flg);
}
// -- Event |=
// IOManager::Event operator|=(const IOManager::Event &l,
//                             const IOManager::Event &r) {
//   return IOManager::Event(static_cast<uint32_t>(l) |
//   static_cast<uint32_t>(r));
// }
void operator|=(IOManager::Event &l, const IOManager::Event &r) {
  l = IOManager::Event(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}
IOManager::Event operator|=(const IOManager::Event &e, uint32_t flg) {
  return (IOManager::Event(static_cast<uint32_t>(e) | flg));
}
// -- Event ~
IOManager::Event operator~(IOManager::Event &e) {
  return IOManager::Event(~(uint32_t)e);
}
// -- Event !
bool operator!(const IOManager::Event &e) {
  if (e == IOManager::Event::NONE)
    return true;
  else
    return false;
}
// -- Event !=
bool operator!=(const IOManager::Event l, const IOManager::Event r) {
  return !(l == r);
}

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

IOManager::FdContext::EventContext &
IOManager::FdContext::getContext(IOManager::Event event) {
  switch (event) {
  case Event::READ:
    return read;
    break;
  case Event::WRITE:
    return write;
    break;
  default:
    APEXSTORM_ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext &ctx) {
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event) {
  // current FdContext containing events include  param: `event`.
  APEXSTORM_ASSERT(events & event);

  // take out the `event` from `events`.
  events = (Event)(events & ~event);
  EventContext &ctx = getContext(event);

  // scheduler the event
  if (ctx.cb) {
    ctx.scheduler->schedule(&ctx.cb);
  } else {
    ctx.scheduler->schedule(&ctx.fiber);
  }
  // reset the event context belonging scheduler
  ctx.scheduler = nullptr;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
  // initialize epoll file descriptor
  m_epfd = epoll_create(5000);
  APEXSTORM_ASSERT(m_epfd > 0);

  // initialize pipe file descriptors
  // m_tickleFds[0] `read`  end of the pipe
  // m_tickleFds[1] `write` end of the pipe
  int rt = pipe(m_tickleFds);
  APEXSTORM_ASSERT(!rt);

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  // event listener interested event, including Read Event and Write Event.
  event.events = EPOLLIN | EPOLLET;
  // its relevant listening file descriptors
  event.data.fd = m_tickleFds[0];

  // `F_SETFL`:    `pipe read end` set to the specified value.
  // `O_NONBLOCK`:  don't wait data while operating in file/socket.
  // if fcntl - F_SETFL is success, return zero, otherwise return -1 and set
  // errno.
  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  APEXSTORM_ASSERT(!rt);

  // add `pipe read end` to epfd, interested in Event::READ or Event::WRITE
  // if epoll_ctl success return zero, otherwise return -1 and set errno.
  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  APEXSTORM_ASSERT(!rt);

  // initialize the socket fd context container.
  contextResize(32);

  // startup the scheduler
  start();
}

IOManager::~IOManager() {
  // stop the scheduler
  stop();

  // close the epoll file descriptor
  close(m_epfd);

  // close the pipe file descriptors
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  // free socket fd container resources
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (m_fdContexts[i]) {
      delete m_fdContexts[i];
    }
  }
}

void IOManager::contextResize(size_t size) {
  m_fdContexts.resize(size);
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {

  // fetch the relevant fd context and storing in `fd_ctx`.if the fd-id is
  // beyond the m_fdContexts.size(), resize and initialize the new ones.
  FdContext *fd_ctx = nullptr;
  RWmutexType::ReadLock lock(m_mutex);
  if ((int)m_fdContexts.size() > fd) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWmutexType::WriteLock lock2(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  // mutex lock the `fd_ctx`
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  // if this fd context interested in `event` for adding
  if (!!(fd_ctx->events & event)) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "addEvent assert fd=" << fd << " event=" << event
        << " fd_ctx.event=" << fd_ctx->events;
    APEXSTORM_ASSERT(!(fd_ctx->events & event));
  }

  // if this fd context has no interested event, set the operation as
  // `EPOLL_CTL_ADD`, otherwise set it as `EPOLL_CTL_MOD` (modify)
  int op = fd_ctx->events != Event::NONE ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  // add event into events collection
  epevent.events = (uint32_t)(EPOLLET | fd_ctx->events | event);
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
        << (EPOLL_EVENTS)epevent.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return -1;
  }

  // atomic pending events count + 1
  ++m_pendingEventCount;

  // modify the `fd context`
  fd_ctx->events = (Event)(fd_ctx->events | event);
  // fetch the `event context` (READ / WRITE)
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);

  // assessing this event_ctx does not have been used or it's already reset.
  // that's mean nothing occupying this fd event context.
  APEXSTORM_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

  // this event belongs to current IOManager
  event_ctx.scheduler = Scheduler::GetThis();
  // solve storing the callback function or fiber
  if (cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();
    // current running fiber, it must in executing state
    APEXSTORM_ASSERT(event_ctx.fiber->getState() == Fiber::State::EXEC);
  }
  return 0;
}

bool IOManager::delEvent(int fd, Event event) {
  // same logic as IOManager::addEvent
  RWmutexType::ReadLock lock(m_mutex);
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) {
    return false;
  }

  // before overloaded operator symbols:
  // Event new_events = (Event)(fd_ctx->events & (Event)(~(int)event));

  // remove the event form the events collection
  Event new_events = (Event)(fd_ctx->events & ~event);
  // corresponding delete operation
  int op = new_events != Event::NONE ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
        << (EPOLL_EVENTS)epevent.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }

  // here we reduce the pending event count
  --m_pendingEventCount;

  // delEvent only remove the event but not trigger this event,
  // if wanting the event will trigger before being deleted.

  // modify this fd events collection as event collection which `event` has
  // removed.
  fd_ctx->events = new_events;
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  // reset the corresponding event(READ / WRITE)
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
  // same logic as IOManager::addEvent
  RWmutexType::ReadLock lock(m_mutex);
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) {
    return false;
  }

  // same as the IOManager::delEvent
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events != Event::NONE ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
        << (EPOLL_EVENTS)epevent.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }

  // acquire the corresponding event context (READ / WRITE)
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  // trigger corresponding event
  fd_ctx->triggerEvent(event);

  // reducing the pending event amount
  --m_pendingEventCount;
  return true;
}

bool IOManager::cancelAll(int fd) {
  // cancel all events in correspond fd context

  // acquire the correspond fd context `fd_ctx`
  // reading lock the IOManager
  RWmutexType::ReadLock lock(m_mutex);
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  // mutex lock the m_fdContext[fd]
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  // fd context does not have interested event
  if (fd_ctx->events == Event::NONE) {
    return false;
  }

  // only delete the event
  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = (uint32_t)Event::NONE;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", "
        << (EPOLL_EVENTS)epevent.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }

  // trigger READ event and WRITE event if marked.
  if (!!(fd_ctx->events & Event::READ)) {
    fd_ctx->triggerEvent(Event::READ);
    --m_pendingEventCount;
  }
  if (!!(fd_ctx->events & Event::WRITE)) {
    fd_ctx->triggerEvent(Event::WRITE);
    --m_pendingEventCount;
  }

  APEXSTORM_ASSERT(fd_ctx->events == Event::NONE);
  return true;
}

IOManager *IOManager::GetThis() {
  return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle() {
  // if scheduler doesn't has idle threads.
  if (!hasIdleThreads()) {
    return;
  }
  // otherwise, write a value("T") into the write end of pipe, in order to
  // notify the IOManager to arrange a thread to handle this task.
  int rt = write(m_tickleFds[1], "T", 1);
  // asserting the write operation is successes
  APEXSTORM_ASSERT(rt == 1);
}

bool IOManager::stopping() {
  // add external condition
  return Scheduler::stopping() && m_pendingEventCount == 0;
}

void IOManager::idle() {
  APEXSTORM_LOG_INFO(g_logger) << "Idle";

  // manage the memory space of `events`
  const uint64_t MAX_EVENTS = 256;
  epoll_event *events = new epoll_event[MAX_EVENTS]();
  std::shared_ptr<epoll_event> shared_events(
      events, [](epoll_event *ptr) { delete[] ptr; });

  while (true) {
    if (stopping()) {
      APEXSTORM_LOG_INFO(g_logger)
          << "name=" << getName() << " idle stopping exit";
      break;
    }

    int rt = 0;
    do {
      static const int MAX_TIMEOUT = 5000;
      // epoll_wait will blocking until:
      // 1. a file descriptor delivers an event;
      // 2. teh call is interrupted by a single handler;
      // 3. the timeout expires(MAX_TIMEOUT milliseconds);
      rt = epoll_wait(m_epfd, events, MAX_EVENTS, MAX_TIMEOUT);

      // on success, return the amount of file descriptors which are in ready
      // state, or zero timeout expires. on failure, return -1 and set the
      // errno indicate the error.

      // APEXSTORM_LOG_INFO(g_logger) << "epoll_wait rt=" << rt;
      if (rt < 0 && errno == EINTR) {
        // failure and interrupt by signal, retry epoll_wait
      } else {
        // timeout or having the amount of `rt` events
        break;
      }
    } while (true);

    // solve `rt` events
    for (int i = 0; i < rt; ++i) {
      // coming events collection
      epoll_event &event = events[i];

      // APEXSTORM_LOG_INFO(g_logger) << event.data.fd;

      // having value in pipe, (sent in IOManager::tickle), take out from read
      // end of pipe
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy;
        while (read(m_tickleFds[0], &dummy, 1) == 1)
          ;
        continue;
      }

      // recover the fd context.
      FdContext *fd_ctx = (FdContext *)event.data.ptr;
      // mutex lock the fd context
      FdContext::MutexType::Lock lock(fd_ctx->mutex);

      // this fd context interested in
      // 1. the socket happen error
      // 2. the socket has been closed
      if (event.events & (EPOLLERR | EPOLLHUP)) {
        // add event interested (read data and write data)
        //  check with the old event collection
        event.events |= (EPOLLIN | EPOLLOUT) & (uint32_t)fd_ctx->events;
      }

      Event real_events = Event::NONE;
      // read data from socket
      if (events->events & EPOLLIN) {
        real_events |= Event::READ;
      }
      // write date from socket
      if (events->events & EPOLLOUT) {
        real_events |= Event::WRITE;
      }

      // check old event collection with READ and WRITE if marked,
      if ((fd_ctx->events & real_events) == Event::NONE) {
        continue;
      }

      // handle the rest event except READ or WRITE
      Event rest_events = (fd_ctx->events & ~real_events);
      int op =
          ((Event)rest_events != Event::NONE) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = (uint32_t)(EPOLLET | rest_events);

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (rt2) {
        APEXSTORM_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd_ctx->fd
            << ", " << (EPOLL_EVENTS)event.events << "):" << rt << " (" << errno
            << ") (" << strerror(errno) << ")";
        continue;
      }

      if (!!(real_events & Event::READ)) {
        fd_ctx->triggerEvent(Event::READ);
        --m_pendingEventCount;
      }
      if (!!(real_events & Event::WRITE)) {
        fd_ctx->triggerEvent(Event::WRITE);
        --m_pendingEventCount;
      }
    }

    // swap out current fiber
    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
  }
}

} // namespace apexstorm