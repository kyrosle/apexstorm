#include "../include/hook.h"
#include "../include/iomanager.h"
#include "../include/log.h"
#include <unistd.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_sleep() {
  apexstorm::IOManager iom(1);
  iom.schedule([]() {
    sleep(2);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 2";
  });

  iom.schedule([]() {
    sleep(3);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 3";
  });
  APEXSTORM_LOG_INFO(g_logger) << "test_sleep";
}

int main() { test_sleep(); }
