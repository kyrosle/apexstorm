#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <atomic>
#include <pthread.h>
#include <semaphore.h>

namespace apexstorm {

// Cpp RAII for managing mutex lock lifetime scope.
template <class T> class ScopedLockImpl {
public:
  // Construct a new Scoped Lock, lock the mutex and set `m_locked` true.
  ScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }

  // Destroy the Scoped Lock Impl, and unlock the mutex.
  ~ScopedLockImpl() { unlock(); }

  // lock the mutex when the mutex is unlocked.
  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  // unlock the mutex when the mutex is locked.
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  // Mutex, SpinLock, CASLock
  T &m_mutex;
  // locking status
  bool m_locked;
};

// Mutex encapsulation, using system api: `pthread_mutex_xxx`.
class Mutex {
public:
  typedef ScopedLockImpl<Mutex> Lock;

  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  void lock() { pthread_mutex_lock(&m_mutex); }
  void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
  // mutex structure
  pthread_mutex_t m_mutex;
};

// Testing mutex for Mutex
class NullMutex {
public:
  typedef ScopedLockImpl<NullMutex> Lock;

  NullMutex() {}
  ~NullMutex() {}

  void lock() {}
  void unlock() {}
};

// Cpp RAII for managing read-write mutex lock in `read` mode lifetime scope.
template <class T> class ReadScopedLockImpl {
public:
  // Construct a new Read Scoped Lock, and lock m_lock in read mode.
  // @param mutex   RWLock class
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }

  // Destroy the Read Scoped Lock, and unlock m_lock.
  ~ReadScopedLockImpl() { unlock(); }

  // lock the rwlock in read mode.
  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  // unlock the rwlock.
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  // rwlock structure
  T &m_mutex;
  // locking status
  bool m_locked;
};

// Cpp RAII for managing read-write mutex lock in `write` mode lifetime scope.
template <class T> class WriteScopedLockImpl {
public:
  // Construct a new Write Scoped Lock Impl object
  // @param mutex RWLock class
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }

  // Destroy the Write Scoped Lock, and unlock the m_rwlock.
  ~WriteScopedLockImpl() { unlock(); }

  // lock the rwlock in `write` mode.
  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  // unlock the rwlock.
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  // rwlock structure
  T &m_mutex;
  // locking status
  bool m_locked;
};

// RWMutex encapsulation, using system api: `pthread_rwlock_xxx`.
class RWMutex {
public:
  typedef ReadScopedLockImpl<RWMutex> ReadLock;
  typedef WriteScopedLockImpl<RWMutex> WriteLock;

  RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

  void rdlock() { pthread_rwlock_rdlock(&m_lock); }
  void wrlock() { pthread_rwlock_wrlock(&m_lock); }
  void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
  pthread_rwlock_t m_lock;
};

// Testing mutex for RWMutex
class NullRWLock {
public:
  typedef ReadScopedLockImpl<NullMutex> ReadLock;
  typedef WriteScopedLockImpl<NullMutex> WriteLock;

  NullRWLock() {}
  ~NullRWLock() {}

  void rdlock() {}
  void wrlock() {}
  void unlock() {}
};

// SpinLock encapsulation, using system api: `pthread_spin_xxx`.
class SpinLock {
public:
  typedef ScopedLockImpl<SpinLock> Lock;

  SpinLock() { pthread_spin_init(&m_lock, 0); }
  ~SpinLock() { pthread_spin_destroy(&m_lock); }

  void lock() { pthread_spin_lock(&m_lock); }
  void unlock() { pthread_spin_unlock(&m_lock); }

private:
  pthread_spinlock_t m_lock;
};

// CASLock(Compare and Swap) encapsulation, using atomic variables.
// using `memory ordering semantics`:
//   `relaxed`, `consume`, `acquire`, `release`, `acq_rel` , `seq_cst`.
class CASLock {
public:
  typedef ScopedLockImpl<CASLock> Lock;

  // set the atomic_flag as clear.
  CASLock() { m_mutex.clear(); }
  // Because atomic variables do not allocate dynamic memory.
  ~CASLock() {}

  void lock() {
    // try to set `set` to m_mutex
    while (std::atomic_flag_test_and_set_explicit(
        &m_mutex, std::memory_order::memory_order_acquire))
      ;
  }
  void unlock() {
    std::atomic_flag_clear_explicit(&m_mutex,
                                    std::memory_order::memory_order_release);
  }

private:
  // atomic variables like bool: set / clear
  volatile std::atomic_flag m_mutex;
};

// Semaphore encapsulation, using system api: `sem_xxx`.
class Semaphore {
public:
  // semaphore init with initial count.
  Semaphore(uint32_t count = 0);
  // destroy the semaphore
  ~Semaphore();

  // semaphore wait
  void wait();
  // semaphore post
  void notify();

private:
  // avoid being copied and moved
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

private:
  sem_t m_semaphore;
};
} // namespace apexstorm
#endif