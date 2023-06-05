#include "../include/apexstorm.h"
#include "log.h"
#include "macro.h"
#include "util.h"
#include <assert.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_assert() {
  APEXSTORM_LOG_INFO(g_logger) << apexstorm::BacktraceToString(10);
  APEXSTORM_ASSERT(1 == 0);
  // APEXSTORM_ASSERT2(1 == 0, "abcdef xx");
}
int main() { test_assert(); }