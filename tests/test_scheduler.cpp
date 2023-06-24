#include "apexstorm.h"
#include <unistd.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_fiber() {
  static int s_count = 5;
  APEXSTORM_LOG_INFO(g_logger) << "[test in fiber] s_count =" << s_count;
  sleep(1);
  if (--s_count >= 0) {
    apexstorm::Scheduler::GetThis()->schedule(&test_fiber,
                                              apexstorm::GetThreadId());
  }
}

int main() {
  APEXSTORM_LOG_INFO(g_logger) << "main";
  apexstorm::Scheduler sc(3, false, "test");
  sc.start();
  APEXSTORM_LOG_INFO(g_logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.stop();
  APEXSTORM_LOG_INFO(g_logger) << "over";
}