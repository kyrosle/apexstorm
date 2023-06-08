#ifndef __APEXSTORM_IOMANAGER_H__
#define __APEXSTORM_IOMANAGER_H__

#include "fiber.h"
#include "mutex.h"
#include "scheduler.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <vector>

namespace apexstorm {

// IO coroutine scheduler basing on epoll.
class IOManager : public Scheduler {
public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWmutexType;

  // IO event
  enum class Event : uint32_t {
    NONE = 0x0, // No Event
    READ = 0x1, // Read Event (EPOLLIN)
    WRITE = 0x4 // Write Event (EPOLLOUT)
  };

private:
  // Socket Event Context
  struct FdContext {
    typedef Mutex MutexType;

    // Event Context
    struct EventContext {
      // event executor scheduler
      Scheduler *scheduler = nullptr;
      // event fiber
      Fiber::ptr fiber;
      // event callback function
      std::function<void()> cb;
    };

    // Get the event context, event type: READ, WRITE, NONE.
    EventContext &getContext(Event event);

    // Reset the event context.
    void resetContext(EventContext &ctx);

    // Trigger the event.
    void triggerEvent(Event event);

    // read event context
    EventContext read;
    // write event context
    EventContext write;
    // event relevant file descriptor
    int fd;
    // current registered events
    Event events = Event::NONE;

    MutexType mutex;
  };

public:
  // Constructor
  // @param: threads - thread amount.
  // @param: use_caller - whether include current thread.
  // @param: name - scheduler name.
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");

  // Destructor, close epoll file descriptor, close pipe handlers, close fd
  // contexts.
  ~IOManager();

  // Add the event
  // @param: fd - socket file descriptor.
  // @param: event - event type.
  // @param: cb - callback function.
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

  // Delete the event, it will not trigger event.
  // @param: fd - socket file descriptor.
  // @param: event - event type.
  bool delEvent(int fd, Event event);

  // Cancel the event, it will trigger event if the event exists.
  // @param: fd - socket file descriptor.
  // @param: event - event type.
  bool cancelEvent(int fd, Event event);

  // Cancel all events.
  // @param: fd - socket file descriptor.
  bool cancelAll(int fd);

  // Return current IOManager instance.
  static IOManager *GetThis();

protected:
  void tickle() override;
  bool stopping() override;
  void idle() override;

  // Resize the socket file descriptors container.
  void contextResize(size_t size);

private:
  // epoll file descriptor
  int m_epfd = 0;
  // pipe file descriptors
  // IOManager holding the read end of pipe, waiting value from the pipe.
  int m_tickleFds[2];
  // the amount of events waiting to execute
  std::atomic<size_t> m_pendingEventCount = {0};
  // IOManager mutex
  RWmutexType m_mutex;
  // socket event contexts container.
  std::vector<FdContext *> m_fdContexts;
};

} // namespace apexstorm

#endif