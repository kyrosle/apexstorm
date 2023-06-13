#include "thread.h"
#include "log.h"
#include "util.h"
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOWN";

Thread *Thread::GetThis() { return t_thread; }

const std::string &Thread::GetName() { return t_thread_name; }

void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name) {
  if (name.empty()) {
    m_name = "UNKNOWN";
  }
  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (rt) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "pthread_create thread failed, rt=" << rt << ", name=" << name;
    throw std::logic_error("pthread_create error");
  }
  // wait for thread created finished
  m_semaphore.wait();
}

Thread::~Thread() {
  if (m_thread) {
    pthread_detach(m_thread);
  }
}

void Thread::join() {
  if (m_thread) {
    int rt = pthread_join(m_thread, nullptr);
    if (rt) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "pthread_join thread failed, rt=" << rt << ", name=" << m_name;
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void Thread::detach() {
  if (m_thread) {
    int rt = pthread_detach(m_thread);
    if (rt) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "pthread_detach thread failed, rt=" << rt << ", name=" << m_name;
      throw std::logic_error("pthread_detach error");
    }
    m_thread = 0;
  }
}

void *Thread::run(void *arg) {
  Thread *thread = (Thread *)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = apexstorm::GetThreadId();
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->m_cb);

  // notify the creator thread
  thread->m_semaphore.notify();
  cb();

  return 0;
}

} // namespace apexstorm