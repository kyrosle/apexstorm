#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "fiber.h"
#include "thread.h"
#include <atomic>
#include <boost/mpl/protect.hpp>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace apexstorm {

// Coroutine scheduler,
// encapsulates an N-M coroutine scheduler,
// there is a thread pool inside, which supports coroutines to switch in the
// thread pool.
class Scheduler {
public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;

  // Constructor default creates a thread, and use caller, and the thread pool
  // name is empty.
  Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");

  // destructor can be reimplemented by subclass.
  virtual ~Scheduler();

  // Return the thread pool name.
  const std::string &getName() const { return m_name; }

  // Get current thread pool reference.
  static Scheduler *GetThis();

  // Get the current main fiber.
  static Fiber *GetMainFiber();

  // thread pool start.
  void start();

  // thread pool stop.
  void stop();

  // schedule `Fiber` or `Callback Function`.
  template <class FiberOrCb> void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread);
    }

    if (need_tickle) {
      tickle();
    }
  }

  // batch scheduling coroutine.
  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin) || need_tickle;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

protected:
  // Notify the coroutine scheduler that there is a task.
  virtual void tickle();
  // Coroutine schedule function.
  void run();
  // Return whether it can be stopped.
  virtual bool stopping();
  // Idle the scheduler
  virtual void idle();
  // Set the current scheduler.
  void setThis();

private:
  // Coroutine scheduling start (lock-free).
  template <class FiberOrCb> bool scheduleNoLock(FiberOrCb fc, int thread) {
    bool need_tickle = m_fibers.empty();
    FiberAndThread ft(fc, thread);
    if (ft.fiber || ft.cb) {
      m_fibers.push_back(ft);
    }
    return need_tickle;
  }

private:
  // Wrapping fiber or callback function, and its specified thread.
  struct FiberAndThread {
    // fiber
    Fiber::ptr fiber;
    // callback function
    std::function<void()> cb;
    // thread id
    int thread;

    // set fiber as f
    FiberAndThread(Fiber::ptr f, int thread_id) : fiber(f), thread(thread_id) {}

    // swap the fiber as f referencing object
    FiberAndThread(Fiber::ptr *f, int thread_id) : thread(thread_id) {
      // avoiding reference counting add 1
      fiber.swap(*f);
    }

    // set the callback function as c
    FiberAndThread(std::function<void()> c, int thread_id)
        : cb(c), thread(thread_id) {}

    // swap the callback function as c referencing object
    FiberAndThread(std::function<void()> *c, int thread_id)
        : thread(thread_id) {
      // avoiding reference counting add 1
      cb.swap(*c);
    }

    // for stl container initialization
    FiberAndThread() : thread(-1) {}

    // reset the fiber and callback function
    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

private:
  // Mutex lock
  MutexType m_mutex;
  // thread pool
  std::vector<Thread::ptr> m_threads;
  // fibers manage
  std::list<FiberAndThread> m_fibers;
  // root fiber
  Fiber::ptr m_rootFiber;
  // thread pool name
  std::string m_name;

protected:
  // threads id array
  std::vector<int> m_threadIds;
  // thread count
  size_t m_threadCount = 0;
  // active thread count
  std::atomic<size_t> m_activeThreadCount = {0};
  // idle thread count
  std::atomic<size_t> m_idleThreadCount = {0};
  // whether is stopping
  bool m_stopping = true;
  // whether auto stop
  bool m_autoStop = false;
  // root thread (use_caller)
  int m_rootThread = 0;
};

} // namespace apexstorm

#endif