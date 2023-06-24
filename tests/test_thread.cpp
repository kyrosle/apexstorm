/**
 * @file test_thread.cpp
 * @brief testing thread module.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "config.h"
#include "log.h"
#include "thread.h"
#include "util.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// switch test direction
// #define TEST_LOG
// #define TEST_MUTEX
// #define TEST_RW_MUTEX
#define TEST_MUTEX_LOG

#if defined(TEST_MUTEX) || defined(TEST_RW_MUTEX)
int count = 0;
#endif

#ifdef TEST_RW_MUTEX
apexstorm::RWMutex s_mutex;
#endif

#ifdef TEST_MUTEX
apexstorm::Mutex s_mutex;
#endif

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

#ifdef TEST_LOG
void fun() {
  APEXSTORM_LOG_INFO(g_logger)
      << "name: " << apexstorm::Thread::GetName()
      << " this.name: " << apexstorm::Thread::GetThis()->getName()
      << " id: " << apexstorm::GetThreadId()
      << " this.id: " << apexstorm::Thread::GetThis()->getId();

  sleep(5);
}
#endif

#ifdef TEST_MUTEX
void fun() {
  for (int i = 0; i < 100; ++i) {
    apexstorm::Mutex::Lock lock(s_mutex);
    ++count;
    // sleep 5ns
    std::this_thread::sleep_for(std::chrono::nanoseconds(500));
  }
}
#endif

#ifdef TEST_RW_MUTEX
void fun() {
  for (int i = 0; i < 100; i++) {
    // apexstorm::RWMutex::WriteLock lock(s_mutex);
    apexstorm::RWMutex::ReadLock lock(s_mutex);
    ++count;
    // sleep 5ns
    std::this_thread::sleep_for(std::chrono::nanoseconds(500));
  }
}
#endif

#ifdef TEST_MUTEX_LOG
void fun1() {
  while (true) {
    APEXSTORM_LOG_INFO(g_logger)
        << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    // sleep 5ns
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void fun2() {
  while (true) {
    APEXSTORM_LOG_INFO(g_logger)
        << "=================================================================";
    // sleep 5ns
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
#endif

int main() {
#ifdef TEST_LOG
  APEXSTORM_LOG_INFO(g_logger) << "thread test log begin";
  std::vector<apexstorm::Thread::ptr> thrs;
  for (int i = 0; i < 5; i++) {
    apexstorm::Thread::ptr thr(
        new apexstorm::Thread(&fun, "name_" + std::to_string(i)));
    thrs.push_back(thr);
  }

  for (int i = 0; i < 5; i++) {
    thrs[i]->join();
  }
  APEXSTORM_LOG_INFO(g_logger) << "thread test log end";
#endif

#ifdef TEST_MUTEX
  APEXSTORM_LOG_INFO(g_logger) << "thread test mutex begin";
  std::vector<apexstorm::Thread::ptr> thrs;
  for (int i = 0; i < 5; i++) {
    apexstorm::Thread::ptr thr(
        new apexstorm::Thread(&fun, "name_" + std::to_string(i)));
    thrs.push_back(thr);
  }

  for (int i = 0; i < 5; i++) {
    thrs[i]->join();
  }

  APEXSTORM_LOG_INFO(g_logger) << "thread test mutex count: " << count;

  APEXSTORM_LOG_INFO(g_logger) << "thread test mutex end";
#endif

#ifdef TEST_RW_MUTEX
  APEXSTORM_LOG_INFO(g_logger) << "thread test rw_mutex begin";
  std::vector<apexstorm::Thread::ptr> thrs;
  for (int i = 0; i < 5; i++) {
    apexstorm::Thread::ptr thr(
        new apexstorm::Thread(&fun, "name_" + std::to_string(i)));
    thrs.push_back(thr);
  }

  for (int i = 0; i < 5; i++) {
    thrs[i]->join();
  }

  APEXSTORM_LOG_INFO(g_logger) << "thread test rw_mutex count: " << count;

  APEXSTORM_LOG_INFO(g_logger) << "thread test rw_mutex end";
#endif

#ifdef TEST_MUTEX_LOG
  APEXSTORM_LOG_INFO(g_logger) << "thread test log mutex begin";
  YAML::Node root =
      YAML::LoadFile("/home/kyros/WorkStation/CPP/apexstorm/conf/log2.yml");
  apexstorm::Config::LoadFromYaml(root);
  std::vector<apexstorm::Thread::ptr> thrs;
  for (int i = 0; i < 5; i++) {
    apexstorm::Thread::ptr thr1(
        new apexstorm::Thread(&fun1, "name_x_" + std::to_string(i)));
    apexstorm::Thread::ptr thr2(
        new apexstorm::Thread(&fun2, "name_=_" + std::to_string(i)));
    thrs.push_back(thr1);
    thrs.push_back(thr2);
  }

  for (int i = 0; i < thrs.size(); i++) {
    thrs[i]->join();
  }

  APEXSTORM_LOG_INFO(g_logger) << "thread test log mutex end";
#endif
}