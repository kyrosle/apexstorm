#include "../include/apexstorm.h"
#include "fiber.h"
#include "thread.h"
#include <string>
#include <vector>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void run_in_fiber() {
  APEXSTORM_LOG_INFO(g_logger) << "run_in_fiber begin";
  apexstorm::Fiber::YieldToReady();
  APEXSTORM_LOG_INFO(g_logger) << "run_in_fiber end";
  apexstorm::Fiber::YieldToHold();
}

void test_fiber() {
  APEXSTORM_LOG_INFO(g_logger) << "main begin -1";
  apexstorm::Fiber::GetThis();
  {
    APEXSTORM_LOG_INFO(g_logger) << "main begin";
    apexstorm::Fiber::ptr fiber(new apexstorm::Fiber(run_in_fiber));
    fiber->call();
    APEXSTORM_LOG_INFO(g_logger) << "main after swapIn";
    fiber->call();
    APEXSTORM_LOG_INFO(g_logger) << "main after end";
    fiber->call();
  }
  APEXSTORM_LOG_INFO(g_logger) << "main after end 2";
}

int main() {
  apexstorm::Thread::SetName("main");
  std::vector<apexstorm::Thread::ptr> thrs;
  for (int i = 0; i < 1; ++i) {
    thrs.push_back(apexstorm::Thread::ptr(
        new apexstorm::Thread(&test_fiber, "name_" + std::to_string(i))));
  }

  for (auto &t : thrs) {
    t->join();
  }
  return 0;
}