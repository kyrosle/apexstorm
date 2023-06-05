#ifndef __APEXSTORM_MACRO_H__
#define __APEXSTORM_MACRO_H__

#include "util.h"
#include <assert.h>
#include <string.h>

#define APEXSTORM_ASSERT(x)                                                    \
  if (!(x)) {                                                                  \
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())                                  \
        << "\n-- -- -- -- -- -- -- --\n"                                       \
        << "*  Assertion: " #x << "\n-- backtrace:\n"                          \
        << apexstorm::BacktraceToString(100, 2, "*\t")                         \
        << "-- -- -- -- -- -- -- --";                                          \
    assert(x);                                                                 \
  }

#define APEXSTORM_ASSERT2(x, w)                                                \
  if (!(x)) {                                                                  \
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())                                  \
        << "\n-- -- -- -- -- -- -- --\n"                                       \
        << "*  Assertion: " #x << "\n-- message:\n*  " << w                    \
        << "\n-- backtrace:\n"                                                 \
        << apexstorm::BacktraceToString(100, 2, "*\t")                         \
        << "-- -- -- -- -- -- -- --";                                          \
    assert(x);                                                                 \
  }

#endif
