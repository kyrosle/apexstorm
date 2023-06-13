#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include "util.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <sys/types.h>
#include <ucontext.h>

namespace apexstorm {

// logger in current module.
static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

// allocator for fiber id.
static std::atomic<uint64_t> s_fiber_id = {0};
// total amount of fibers.
static std::atomic<uint64_t> s_fiber_count = {0};

// current thread running fiber pointer.
static thread_local Fiber *t_fiber = nullptr;
// current thread temporary storage fiber object.
static thread_local Fiber::ptr t_threadFiber = nullptr;

// set fiber stack_size config
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 128 * 1024, "fiber stack size");

// Allocator encapsulation
class MallocStackAllocator {
public:
  static void *Alloc(size_t size) { return malloc(size); }
  static void Dealloc(void *vp, size_t size) { return free(vp); }
};
using StackALloc = MallocStackAllocator;

// --- constructor and destructor

Fiber::Fiber() {
  m_state = State::EXEC;
  SetThis(this);

  // store current context in `m_ctx`
  if (getcontext(&m_ctx)) {
    APEXSTORM_ASSERT2(false, "getcontext");
  }

  // s_fiber_count.fetch_add(1);
  ++s_fiber_count;

  APEXSTORM_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {

  ++s_fiber_count;

  // set the fiber stack size, param or config default.
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackALloc::Alloc(m_stacksize);

  // store current context in `m_ctx`
  if (getcontext(&m_ctx)) {
    APEXSTORM_ASSERT2(false, "getcontext");
  }
  // exit program after executed
  m_ctx.uc_link = nullptr;
  // context stack pointer
  m_ctx.uc_stack.ss_sp = m_stack;
  // context stack size
  m_ctx.uc_stack.ss_size = m_stacksize;

  // modify `m_ctx` with context settings above and the execution function
  // `Fiber::MainFunc`.
  if (!use_caller) {
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
  }

  APEXSTORM_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

Fiber::~Fiber() {

  --s_fiber_count;

  // dealloc the context stack when current fiber in TERM | INIT | EXCEPT status
  if (m_stack) {
    APEXSTORM_ASSERT(m_state == State::TERM || m_state == State::INIT ||
                     m_state == State::EXCEPT);

    StackALloc::Dealloc(m_stack, m_stacksize);
  } else {
    // otherwise, current fiber does not have callback function and in EXEC
    // status, it meets at current thread root fiber.
    APEXSTORM_ASSERT(!m_cb);
    APEXSTORM_ASSERT(m_state == State::EXEC);

    // `cur` point at local thread running `t_fiber`
    Fiber *cur = t_fiber;

    // remove self pointer in `t_fiber`
    if (cur == this) {
      SetThis(nullptr);
    }
  }

  APEXSTORM_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
}

void Fiber::reset(std::function<void()> cb) {
  // current fiber context exists.
  APEXSTORM_ASSERT(m_stack);
  // current fiber in TERM | INIT | EXCEPT status
  APEXSTORM_ASSERT(m_state == State::TERM || m_state == State::INIT ||
                   m_state == State::EXCEPT);

  m_cb = cb;

  // modify `m_ctx`
  if (getcontext(&m_ctx)) {
    APEXSTORM_ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);

  // reset fiber status INIT
  m_state = State::INIT;
}

void Fiber::call() {
  // current thread run this fiber.
  SetThis(this);
#ifdef __LOG_FIBER_CHANGED
  APEXSTORM_LOG_INFO(g_logger)
      << "call: " << t_threadFiber->getId() << " -> " << this->getId();
#endif
  m_state = State::EXEC;

  // current fiber -> this.m_ctx
  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
    APEXSTORM_ASSERT2(false, "swapcontext");
  }
}

void Fiber::back() {
  // swap scheduler arranged fiber, and storing current context in this fiber
  // m_ctx.
  SetThis(t_threadFiber.get());
#ifdef __LOG_FIBER_CHANGED
  APEXSTORM_LOG_INFO(g_logger)
      << "back: " << getId() << " -> " << t_threadFiber->getId();
#endif
  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    APEXSTORM_ASSERT2(false, "swapcontext");
  }
}

void Fiber::swapIn() {
  // current thread run this fiber.
  SetThis(this);

#ifdef __LOG_FIBER_CHANGED
  APEXSTORM_LOG_INFO(g_logger)
      << "swapIn: " << getId() << " -> " << Scheduler::GetMainFiber()->getId();
#endif
  APEXSTORM_ASSERT(m_state != State::EXEC);
  m_state = State::EXEC;

  // swap this fiber ucontext, and storing current context into
  // scheduler.prev_fiber.ucontext.

  // scheduler.fiber.m_ctx -> this.m_ctx
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    APEXSTORM_ASSERT2(false, "swapcontext");
  }
}

void Fiber::swapOut() {
  // current thread run the scheduler arranged fiber.
  SetThis(Scheduler::GetMainFiber());
#ifdef __LOG_FIBER_CHANGED
  APEXSTORM_LOG_INFO(g_logger)
      << "swapOut: " << Scheduler::GetMainFiber()->getId() << " -> " << getId();
#endif
  // swap scheduler arranged fiber, and storing current context in this fiber
  // m_ctx.
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    APEXSTORM_ASSERT2(false, "swapcontext");
  }
}

void Fiber::SetThis(Fiber *f) { t_fiber = f; }

Fiber::ptr Fiber::GetThis() {
  // if this fiber is not the root fiber in current thread.
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }

  // create a new root fiber, and return root fiber.
  // (root fiber doesn't have m_cb callback function)
  Fiber::ptr main_fiber(new Fiber);
  APEXSTORM_ASSERT(main_fiber.get() == t_fiber);
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  cur->m_state = State::READY;
  if (apexstorm::Scheduler::GetMainFiber()) {
    cur->swapOut();
  } else {
    cur->back();
  }
}

void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  cur->m_state = State::HOLD;
  if (apexstorm::Scheduler::GetMainFiber()) {
    cur->swapOut();
  } else {
    cur->back();
  }
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc() { // -> !
  // ptr reference counts always > 1
  // root fiber must exist.
  Fiber::ptr cur = GetThis();
  APEXSTORM_ASSERT(cur);

  try {
    // execute callback function
    cur->m_cb();
    // set nullptr to m_cb(after execution)
    cur->m_cb = nullptr;
    // set fiber into termination status
    cur->m_state = State::TERM;
  } catch (std::exception &ex) {
    // set fiber into exception status
    // and take log in system block.
    cur->m_state = State::EXCEPT;

    APEXSTORM_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << std::endl
                                  << "fiber id=" << cur->getId() << std::endl
                                  << apexstorm::BacktraceToString();
  } catch (...) {
    // unexpected and unpredictable
    cur->m_state = State::EXCEPT;

    APEXSTORM_LOG_ERROR(g_logger) << "Fiber Except" << std::endl
                                  << "fiber id=" << cur->getId() << std::endl
                                  << apexstorm::BacktraceToString();
  }

  // -- after callback function finish execution --

  // take the raw pointer
  auto raw_ptr = cur.get();
  // smart pointer reference counting - 1
  cur.reset();
  // this fiber will be destructed calling ~Fiber.
  if (Scheduler::GetMainFiber()) {
    raw_ptr->swapOut();
  } else if (t_threadFiber) {
    raw_ptr->back();
  } else {
    APEXSTORM_ASSERT2(false, "no runnable fiber fiber_id=" +
                                 std::to_string(raw_ptr->getId()));
  }

  APEXSTORM_ASSERT2(false,
                    "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() { // -> !
  // ptr reference counts always > 1
  // root fiber must exist.
  Fiber::ptr cur = GetThis();
  APEXSTORM_ASSERT(cur);

  try {
    // execute callback function
    cur->m_cb();
    // set nullptr to m_cb(after execution)
    cur->m_cb = nullptr;
    // set fiber into termination status
    cur->m_state = State::TERM;
  } catch (std::exception &ex) {
    // set fiber into exception status
    // and take log in system block.
    cur->m_state = State::EXCEPT;

    APEXSTORM_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << std::endl
                                  << "fiber id=" << cur->getId() << std::endl
                                  << apexstorm::BacktraceToString();
  } catch (...) {
    // unexpected and unpredictable
    cur->m_state = State::EXCEPT;

    APEXSTORM_LOG_ERROR(g_logger) << "Fiber Except" << std::endl
                                  << "fiber id=" << cur->getId() << std::endl
                                  << apexstorm::BacktraceToString();
  }

  // -- after callback function finish execution --

  // take the raw pointer
  auto raw_ptr = cur.get();
  // smart pointer reference counting - 1
  cur.reset();
  // this fiber will be destructed calling ~Fiber.
  raw_ptr->back();

  APEXSTORM_ASSERT2(false,
                    "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

u_int64_t Fiber::GetFiberId() {
  if (t_fiber) {
    return t_fiber->getId();
  }
  return 0;
}

} // namespace apexstorm