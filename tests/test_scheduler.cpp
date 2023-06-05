#include "../include/apexstorm.h"
#include "fiber.h"
#include "log.h"
#include <unistd.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_fiber() {
  APEXSTORM_LOG_INFO(g_logger) << "[test in fiber]";
  sleep(1);
  apexstorm::Scheduler::GetThis()->schedule(&test_fiber);
}

int main() {
  APEXSTORM_LOG_INFO(g_logger) << "main";
  apexstorm::Scheduler sc(3, true, "test");
  sc.start();
  APEXSTORM_LOG_INFO(g_logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.stop();
  APEXSTORM_LOG_INFO(g_logger) << "over";
}