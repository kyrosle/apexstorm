#ifndef __APEXSTORM_FIBER_H__
#define __APEXSTORM_FIBER_H__

#include "thread.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <ucontext.h>

// #define __LOG_FIBER_CHANGED
namespace apexstorm {

class Scheduler;

// Coroutine class.
class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

public:
  typedef std::shared_ptr<Fiber> ptr;

  // Coroutine status
  enum class State {
    // Initial state
    INIT,
    // Pause state
    HOLD,
    // Executing state
    EXEC,
    // Terminating state
    TERM,
    // Executable state
    READY,
    // Abnormal state
    EXCEPT
  };

private:
  // Constructor, create the first coroutine per thread.
  Fiber();

public:
  // Constructor,
  // @param cb          Functions executed by coroutines
  // @param stacksize   coroutine stack size
  // @param use_caller  whether to use caller thread
  Fiber(std::function<void()> cb, size_t stacksize = 0,
        bool use_caller = false);

  // destructor
  ~Fiber();

  // Reset fiber function
  // @pre     getState() == INIT | TERM | EXCEPT
  // @post    getState() == INIT
  void reset(std::function<void()> cb);

  // Switch the current coroutine to the running state.
  // @pre   getState() != EXEC
  // @post  getState() == EXEC
  void swapIn();

  // Switch the current thread to the execution state.
  // @pre the main coroutine executing for the current thread.
  void swapOut();

  // same as swapIn(), set current coroutine to target executor thread.
  void call();

  // same as swapOut(), set current coroutine to target executor thread.
  void back();

  // Get the coroutine id.
  u_int64_t getId() const { return m_id; }

  // Get the coroutine status.
  State getState() const { return m_state; }

public:
  // Set the running coroutine of the current thread.
  static void SetThis(Fiber *f);

  // Returns the coroutine currently in.
  static Fiber::ptr GetThis();

  // Yield the current fiber, and set Ready status.
  // @post getState() == READY
  static void YieldToReady();

  // Yield the current fiber, and set Hold status.
  // @post getState() == HOLD
  static void YieldToHold();

  // Returns the total number of current coroutines.
  static uint64_t TotalFibers();

  // Coroutine execution function, -> !
  // @post return to the main fiber of thread
  static void MainFunc();
  // Coroutine execution function(used by caller thread), -> !
  // @post return to the main fiber of thread
  static void CallerMainFunc();

  // Return the current fiber id.
  static uint64_t GetFiberId();

private:
  // fiber id
  uint64_t m_id = 0;
  // fiber stack size
  uint32_t m_stacksize = 0;
  // fiber status
  State m_state = State::INIT;
  // fiber context
  ucontext_t m_ctx;
  // fiber stack pointer
  void *m_stack = nullptr;
  // fiber execute function
  std::function<void()> m_cb;
};

} // namespace apexstorm

#endif