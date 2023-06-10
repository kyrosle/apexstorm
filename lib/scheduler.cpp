#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "thread.h"
#include "util.h"
#include <functional>
#include <string>
#include <vector>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

// current thread scheduler pointer.
static thread_local Scheduler *t_scheduler = nullptr;
// current fiber pointer scheduled to run.
static thread_local Fiber *t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name) {
  APEXSTORM_ASSERT(threads > 0);

  // Indicates whether the thread calling this constructor should be used as a
  // worker thread for the coroutine scheduler.
  if (use_caller) {
    // scheduler will create a new fiber as main fiber(treating as a thread),
    // used for scheduling other fibers.
    apexstorm::Fiber::GetThis();
    --threads;

    APEXSTORM_ASSERT(GetThis() == nullptr);
    t_scheduler = this;

    // root fiber execute Scheduler::run()
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));

    apexstorm::Thread::SetName(m_name);

    // the first scheduled fiber is root fiber.
    t_fiber = m_rootFiber.get();
    // this scheduler is bind the corresponding thread.
    m_rootThread = apexstorm::GetThreadId();
  } else {

    // can be moved to another thread
    m_rootThread = -1;
  }

  // record the threads pool size
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  APEXSTORM_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }

Fiber *Scheduler::GetMainFiber() { return t_fiber; }

void Scheduler::start() {
  { // -- thread safety control
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
      return;
    }
    m_stopping = false;
    APEXSTORM_ASSERT(m_threads.empty());

    // allocate thread pool in `m_threads`
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
      m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                    m_name + "_" + std::to_string(i)));
      // here, we will get the thread ids(are in running state), because we use
      // semaphore to control startup flow.
      m_threadIds.push_back(m_threads[i]->getId());
    }
  } // --

  // swap in the root fiber to do scheduling
  // [question?] but when the root fiber will be nullptr
  // if (m_rootFiber) {
  //   m_rootFiber->call();
  //   APEXSTORM_LOG_INFO(g_logger) << "call out " <<
  //   (int)m_rootFiber->getState();
  // }
}

void Scheduler::stop() {
  // graceful stop scheduler

  // scheduler can slide into stopping status
  m_autoStop = true;
  // entering into stop status:
  // 1. root fiber exist
  // 2. root fiber in TERM or INIT
  // 3. threads pool size == 0
  if (m_rootThread && m_threadCount == 0 &&
      ((m_rootFiber->getState() == Fiber::State::TERM) ||
       m_rootFiber->getState() == Fiber::State::INIT)) {
    APEXSTORM_LOG_INFO(g_logger) << this << " stopped";
    m_stopping = true;

    // fiber[] empty && active thread count == 0
    if (stopping()) {
      return;
    }
  }

  // bool exit_on_this_fiber = false;
  if (m_rootThread != -1) {
    APEXSTORM_ASSERT(GetThis() == this);
  } else {
    APEXSTORM_ASSERT(GetThis() != this);
  }

  m_stopping = true;
  for (size_t i = 0; i < m_threadCount; ++i) {
    tickle();
  }

  if (m_rootFiber) {
    tickle();
  }

  if (stopping()) {
    return;
  }

  if (m_rootFiber) {
    // while (!stopping()) {
    //   if (m_rootFiber->getState() == Fiber::State::TERM ||
    //       m_rootFiber->getState() == Fiber::State::EXCEPT) {
    //     m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0,
    //     true)); APEXSTORM_LOG_INFO(g_logger) << "root fiber is term, reset";
    //     t_fiber = m_rootFiber.get();
    //   }
    //   m_rootFiber->call();
    // }
    if (!stopping()) {
      m_rootFiber->call();
    }
  }

  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(m_mutex);
    thrs.swap(m_threads);
  }

  for (auto &i : thrs) {
    i->join();
  }

  // if (exit_on_this_fiber) {
  // }
}

void Scheduler::run() {
  APEXSTORM_LOG_INFO(g_logger) << "run";

  Fiber::GetThis();
  setThis();

  // if current thread is not the root fiber, set it.
  if (apexstorm::GetThreadId() != m_rootThread) {
    t_fiber = Fiber::GetThis().get();
  }

  // idle fiber
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

  // create a callback fiber.
  Fiber::ptr cb_fiber;

  // fiber and thread data structure
  FiberAndThread ft;

  // loop scheduling
  while (true) {
    ft.reset();

    // mark whether we should swap the fiber
    bool tickle_me = false;
    bool is_active = false;

    { // search available fiber from `m_fibers`, lock
      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        if (it->thread != -1 && it->thread != apexstorm::GetThreadId()) {
          // having specifed thread and not current thread, pass
          ++it;
          tickle_me = true;
          continue;
        }

        APEXSTORM_ASSERT(it->fiber || it->cb);
        if (it->fiber && it->fiber->getState() == Fiber::State::EXEC) {
          ++it;
          continue;
        }

        ft = *it;
        tickle_me = false;
        m_fibers.erase(it);
        ++m_activeThreadCount;
        is_active = true;
        break;
      }
    } // -- -- -- --

    if (tickle_me) {
      tickle();
    }

    // fiber && fiber is not TERM | EXCEPT
    if (ft.fiber && ft.fiber->getState() != Fiber::State::TERM &&
        ft.fiber->getState() != Fiber::State::EXCEPT) {

      ft.fiber->swapIn();
      --m_activeThreadCount;

      // -- fiber swap back --

      if (ft.fiber->getState() == Fiber::State::READY) {
        // schedule again
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::State::TERM &&
                 ft.fiber->getState() != Fiber::State::EXCEPT) {
        // fiber stay in holding
        ft.fiber->m_state = Fiber::State::HOLD;
      }

      ft.reset();
    } else if (ft.cb) { // callback function

      // swap with `cb_fiber` above, and swapIn
      if (cb_fiber) {
        cb_fiber->reset(ft.cb);
      } else {
        cb_fiber.reset(new Fiber(ft.cb));
        ft.cb = nullptr;
      }
      ft.reset();

      cb_fiber->swapIn();
      --m_activeThreadCount;

      // -- cb_fiber swap back --

      if (cb_fiber->getState() == Fiber::State::READY) {
        schedule(cb_fiber);
        // schedule again
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::State::EXCEPT ||
                 cb_fiber->getState() == Fiber::State::TERM) {
        // this fiber should terminate
        cb_fiber->reset(nullptr);
      } else { // if (cb_fiber->getState() != Fiber::State::TERM)
               // this fiber stay holding
        cb_fiber->m_state = Fiber::State::HOLD;
        cb_fiber.reset();
      }
    } else { // swap into idle fiber
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }

      if (idle_fiber->getState() == Fiber::State::TERM) {
        APEXSTORM_LOG_INFO(g_logger) << "idle fiber term";
        break;
      }

      ++m_idleThreadCount;
      idle_fiber->swapIn();
      --m_idleThreadCount;

      // -- idle fiber swap back --
      if (idle_fiber->getState() != Fiber::State::TERM &&
          idle_fiber->getState() != Fiber::State::EXCEPT) {
        // continue stay holding
        idle_fiber->m_state = Fiber::State::HOLD;
      }
    }
  }
}

void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::tickle() { APEXSTORM_LOG_INFO(g_logger) << "tickle"; }

void Scheduler::idle() {
  APEXSTORM_LOG_INFO(g_logger) << "Idle";
  while (!stopping()) {
    apexstorm::Fiber::YieldToHold();
  }
}

bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  return m_autoStop && m_stopping && m_fibers.empty() &&
         m_activeThreadCount == 0;
}

} // namespace apexstorm