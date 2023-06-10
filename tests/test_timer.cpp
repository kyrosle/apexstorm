#include "../include/apexstorm.h"
#include "../include/iomanager.h"
#include "../include/timer.h"
#include "log.h"

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

static apexstorm::Timer::ptr timer1, timer2;
// static apexstorm::Timer::ptr timer1, timer2;
void test_timer() {
  apexstorm::IOManager iom(1, true, "test");
  timer1 = iom.addTimer(
      500,
      []() {
        static int cnt = 0;
        APEXSTORM_LOG_INFO(g_logger) << "Hello Timer1 static count=" << cnt;
        if (++cnt == 3) {
          timer1->reset(100, true);
        }
      },
      true);
  timer2 = iom.addTimer(
      1000,
      []() {
        static int cnt = 0;
        APEXSTORM_LOG_INFO(g_logger) << "Hello Timer2 static count=" << cnt;
        if (++cnt == 5) {
          timer2->reset(100, true);
        }
      },
      true);
}
int main() { test_timer(); }

// ------------------------------------------------------
// [ERROR] in test_timer(): gdb show error when calling `shared_from_this`
// ------------------------------------------------------
// (1)
// apexstorm::Timer::ptr timer1;
// timer1 = iom.addTimer(
//     500,
//     [timer1]() {
//       static int cnt = 0;
//       APEXSTORM_LOG_INFO(g_logger) << "Hello Timer1 static count=" << cnt;
//       if (++cnt == 3) {
//         timer1->reset(100, true);
//       }
//     },
//     true);
// ------------------------------------------------------
// (2)
// apexstorm::Timer::ptr timer1 = iom.addTimer(
//     500,
//     [&timer1]() {
//       static int cnt = 0;
//       APEXSTORM_LOG_INFO(g_logger) << "Hello Timer1 static count=" << cnt;
//       if (++cnt == 3) {
//         timer1->reset(100, true);
//       }
//     },
//     true);
// ------------------------------------------------------