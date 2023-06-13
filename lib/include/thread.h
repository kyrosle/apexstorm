/**
 * @file thread.h
 * @brief thread, mutex lock, semaphore related encapsulation.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM__THREAD_H__
#define __APEXSTORM__THREAD_H__

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdint.h>
#include <thread>

#include "mutex.h"
#include "noncopyable.h"

// pthread_xxx
// std::thread, <pthread>

namespace apexstorm {

// Thread encapsulation, using system api: `pthread_xxx`.
class Thread : Noncopyable {
public:
  typedef std::shared_ptr<Thread> ptr;

  // create a new thread
  // @param cb execute function
  // @param name thread name
  Thread(std::function<void()> cb, const std::string &name);

  // detach the thread
  ~Thread();

  // Get the threads id
  pid_t getId() const { return m_id; }

  // Get the thread name
  const std::string &getName() const { return m_name; }

  // join with the terminated thread
  void join();

  // detach the thread
  void detach();

  // Get the current thread pointer
  static Thread *GetThis();

  // Get the current thread name
  static const std::string &GetName();

  // Set the current thread name
  static void SetName(const std::string &name);

private:
  // avoid being copied and moved
  Thread(const Thread &) = delete;
  Thread(const Thread &&) = delete;
  Thread &operator=(const Thread &) = delete;

  // thread executor function
  static void *run(void *arg);

private:
  // thread id
  pid_t m_id = -1;
  // thread structure
  pthread_t m_thread = 0;
  // thread execute function
  std::function<void()> m_cb;
  // thread name
  std::string m_name;
  // semaphore structure, using for waiting thread startup
  Semaphore m_semaphore;
};

} // namespace apexstorm

#endif
