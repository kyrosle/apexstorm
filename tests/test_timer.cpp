#include "../include/apexstorm.h"
#include "../include/iomanager.h"
#include "../include/timer.h"
#include "log.h"
#include <memory>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

// static apexstorm::Timer::ptr timer1, timer2;
void test_timer() {
  apexstorm::IOManager iom(10, true, "test");
  apexstorm::Timer::ptr timer1 = iom.addTimer(
      500,
      [&timer1]() {
        static int cnt = 0;
        APEXSTORM_LOG_INFO(g_logger) << "Hello Timer1 static count=" << cnt;
        if (++cnt == 3) {
          timer1->reset(100, true);
        }
        if (cnt == 10) {
          timer1->cancel();
        }
      },
      true);
  apexstorm::Timer::ptr timer2 = iom.addTimer(
      1000,
      [&timer2]() {
        static int cnt = 0;
        APEXSTORM_LOG_INFO(g_logger) << "Hello Timer2 static count=" << cnt;
        if (++cnt == 5) {
          timer2->reset(100, true);
        }
        if (cnt == 10) {
          timer2->cancel();
        }
      },
      true);
}

int main() { test_timer(); }
