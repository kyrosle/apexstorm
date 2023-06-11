#include "timer.h"
#include "iomanager.h"
#include "mutex.h"
#include "util.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace apexstorm {

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
  if (!lhs && !rhs)
    return false;
  if (!lhs) {
    return true;
  }
  if (!rhs) {
    return false;
  }
  if (lhs->m_next < rhs->m_next) {
    return true;
  }
  if (lhs->m_next > rhs->m_next) {
    return false;
  }
  // compare its address
  return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager *manager)
    : m_ms(ms), m_recurring(recurring), m_manager(manager), m_cb(cb) {
  m_next = apexstorm::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {}

bool Timer::cancel() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (m_cb) {
    // set nullptr, it `m_cb` is a shared_ptr, it will reduce reference count
    m_cb = nullptr;
    // search this Timer from TimerManager, and erase it
    auto it = m_manager->m_timers.find(shared_from_this());
    m_manager->m_timers.erase(it);
    return true;
  }
  // empty callback function
  return false;
}

bool Timer::refresh() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    // empty callback function
    return false;
  }

  // search this Timer from TimerManager
  auto it = m_manager->m_timers.find(shared_from_this());
  if (it == m_manager->m_timers.end()) {
    // not found
    return false;
  }
  // erase this Timer and refresh the timer params, and then insert it to the
  // TimerManager again
  auto t_ptr = *it; // [tick]: same as `Timer::reset()`
  m_manager->m_timers.erase(it);
  // postponed the exactly execute time in Timer
  m_next = apexstorm::GetCurrentMS() + m_ms;
  m_manager->m_timers.insert(shared_from_this());
  t_ptr.reset();
  return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
  if (ms == m_ms && !from_now) {
    // delay time is same, and don't have to recalculate from now
    return true;
  }

  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    // callback function is empty
    return false;
  }
  // search this Timer, erase and add it again after modifing the start time and
  // next exactly execution time.
  auto it = m_manager->m_timers.find(shared_from_this());
  // [tricks]: make the shared_ptr reference counts + 1,
  // and then the operation of `std::set<>::erase()` will reduce the shared_ptr
  // reference counts, avoiding this operation causing destructor current
  // object. (otherwise it will core dump when calling `shared_from_this()` ..
  // (UB))
  auto t_ptr = *it;
  if (it == m_manager->m_timers.end()) {
    t_ptr.reset();
    return false;
  }
  m_manager->m_timers.erase(it);
  // reset start time relevant with `from_now`
  uint64_t start = 0;
  if (from_now) {
    start = apexstorm::GetCurrentMS();
  } else {
    start = m_next - m_ms;
  }
  m_ms = ms;
  m_next = start + m_ms;
  m_manager->addTimer(shared_from_this(), lock);
  return true;
}

TimerManager::TimerManager() { m_previousTime = apexstorm::GetCurrentMS(); }

TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
  Timer::ptr timer(new Timer(ms, cb, recurring, this));
  RWMutexType::WriteLock lock(m_mutex);
  addTimer(timer, lock);
  return timer;
}

// Wrapping void() callback function with a weak condition checking,
// binding into a void().
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
  std::shared_ptr<void> tmp = weak_cond.lock();
  if (tmp) {
    cb();
  }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring) {
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  m_tickled = false;
  if (m_timers.empty()) {
    return ~0ull;
  }

  const Timer::ptr &next = *m_timers.begin();
  lock.unlock();
  uint64_t now_ms = apexstorm::GetCurrentMS();
  if (now_ms >= next->m_next) {
    return 0;
  } else {
    return next->m_next - now_ms;
  }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
  uint64_t now_ms = apexstorm::GetCurrentMS();
  // select all expired timers.
  std::vector<Timer::ptr> expired;

  { // value exist?
    RWMutexType::ReadLock lock(m_mutex);
    if (m_timers.empty()) {
      return;
    }
  }
  RWMutexType::WriteLock lock(m_mutex);
  // check again, because we release the read-lock previously, others threads
  // may lock it, and clean the timers set collections, and then it will cause
  // here `m_timers->begin()` to nullptr.
  if (m_timers.empty()) {
    return;
  }

  bool rollover = detectClockRollover(now_ms);
  if (!rollover && (*m_timers.begin())->m_next > now_ms) {
    // system time has not changed, and the timer front of timers list, exactly
    // execution time is over now.
    return;
  }

  // temporary timer using for lower_bound,
  // select all exactly execution time is before now.
  Timer::ptr now_timer(new Timer(now_ms));
  // if system has changed time, take our all timers,
  // otherwise select the timers exactly execution time before now
  auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
  while (it != m_timers.end() && (*it)->m_next == now_ms) {
    ++it;
  }
  // iterator insert
  expired.insert(expired.begin(), m_timers.begin(), it);
  // iterator erase
  m_timers.erase(m_timers.begin(), it);

  // reserve the capitally
  cbs.reserve(expired.size());

  for (auto &timer : expired) {
    // push_back expires timer callback function into param[in] cbs.
    cbs.push_back(timer->m_cb);
    if (timer->m_recurring) {
      // if this expired timer has set mark `recurring`,
      // reinsert into TimerManager
      timer->m_next = now_ms + timer->m_ms;
      m_timers.insert(timer);
    } else {
      // set callback function nullptr,
      // clean the reference count.
      timer->m_cb = nullptr;
    }
  }
}

void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock) {
  // std::set<>::insert() -> (iterator, bool)
  auto it = m_timers.insert(timer).first;
  // mark whether insert the timer to the head of list
  bool at_front = (it == m_timers.begin() && !m_tickled);
  if (at_front) {
    m_tickled = true;
  }
  lock.unlock();
  if (at_front) {
    // execute when the timer is inserted into the head of list
    onTimerInsertedAtFront();
  }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
  bool rollover = false;
  // checking system clock rollover status
  if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
    rollover = true;
  }
  m_previousTime = now_ms;
  return rollover;
}

bool TimerManager::hasTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  return !m_timers.empty();
}

} // namespace apexstorm