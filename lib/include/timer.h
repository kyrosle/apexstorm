#ifndef __APEXSTORM_TIMER_H__
#define __APEXSTORM_TIMER_H__

#include "mutex.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace apexstorm {

class TimerManager;

// Timer
class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

public:
  typedef std::shared_ptr<Timer> ptr;

  // Cancel Timer
  bool cancel();

  // Refresh Timer execution timeout
  bool refresh();

  // Reset TImer timeout
  // @param: ms - timer internally execution timeout
  // @param: from_now - whether recalculate the timer from now
  bool reset(uint64_t ms, bool from_now);

private:
  // Constructor
  // @param: ms - timer internally execution timeout
  // @param: cb - callback function
  // @param: recurring - whether to recycle
  // @param: manager - TimerManager
  Timer(uint64_t ms, std::function<void()> cb, bool recurring,
        TimerManager *manager);

  // Constructor
  // @param: next - the next time execution timestamp(ms)
  Timer(uint64_t next);

private:
  // whether recycle the timer
  bool m_recurring = false;
  // execution cycle time
  uint64_t m_ms = 0;
  // exactly execution time
  uint64_t m_next = 0;
  // callback function
  std::function<void()> m_cb;
  // Timer manager pointer
  TimerManager *m_manager = nullptr;

private:
  // Timer Comparator (using in std::set<>)
  struct Comparator {
    bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
  };
};

// Timer Manager
class TimerManager {
  friend class Timer;

public:
  typedef RWMutex RWMutexType;

  // Constructor
  TimerManager();

  // Destructor
  virtual ~TimerManager();

  // Add timer
  // @param: ms - timer internally execution timeout
  // @param: cb - callback function
  // @param: recurring - whether to recycle the timer
  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                      bool recurring = false);

  // Add Condition timer,  this timer will added into the TimerManager,
  // but if the timer is expired and its delaying resource is not existed, it
  // will do nothing.
  // @param: ms - timer internally execution timeout
  // @param: cb - callback function
  // @param: weak_cond - weak reference to some resource, if this resource is
  // not existed(weak_cond.lock() == nullptr)
  // @param: recurring - whether to recycle the timer
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                               std::weak_ptr<void> weak_cond,
                               bool recurring = false);

  // Time interval to the last timer execution(ms)
  uint64_t getNextTimer();

  // Get a list of callback functions for the timer to execute.
  // @param[out]: cbs - callback functions list
  void listExpiredCb(std::vector<std::function<void()>> &cbs);

  // whether has timer
  bool hasTimer();

protected:
  // When a new timer is inserted into the header of the timer, execute the
  // function.
  virtual void onTimerInsertedAtFront() = 0;

  // add timer to TimerManager
  void addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock);

private:
  // Check if the server time has been adjusted.
  bool detectClockRollover(uint64_t now_ms);

private:
  RWMutexType m_mutex;
  // timers collections
  std::set<Timer::ptr, Timer::Comparator> m_timers;
  // whether trigger `onTimerInsertedAtFront`
  bool m_tickled = false;
  // the last time execute
  uint64_t m_previousTime = 0;
};

} // namespace apexstorm

#endif
