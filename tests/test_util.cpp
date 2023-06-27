#include "apexstorm.h"
#include "log.h"
#include "util.h"
#include <assert.h>
#include <utility>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_assert() {
  // APEXSTORM_LOG_INFO(g_logger) << apexstorm::BacktraceToString(10);
  // APEXSTORM_ASSERT(1 == 0);
  typedef const int &Type1;
  APEXSTORM_LOG_INFO(g_logger) << debug_utils::cpp_type_name<Type1>();

  Type1 type1 = 1;
  APEXSTORM_LOG_INFO(g_logger)
      << debug_utils::cpp_value_type_name(std::move(type1));

  // APEXSTORM_ASSERT2(1 == 0, "abcdef xx");
}
int main() { test_assert(); }