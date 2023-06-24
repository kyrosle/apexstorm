#ifndef __APEXSTORM_MACRO_H__
#define __APEXSTORM_MACRO_H__

#include "util.h"
#include <assert.h>
#include <string.h>

#if defined __GNUC__ || defined __llvm__
#define APEXSTORM_LIKELY(x) __builtin_expect(!!(x), 1)
#define APEXSTORM_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define APEXSTORM_LIKELY(x) (x)
#define APEXSTORM_UNLIKELY(x) (x)
#endif

#define APEXSTORM_ASSERT(x)                                                    \
  if (APEXSTORM_UNLIKELY(!(x))) {                                              \
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())                                  \
        << "\n-- -- -- -- -- -- -- --\n"                                       \
        << "*  Assertion: " #x << "\n-- backtrace:\n"                          \
        << apexstorm::BacktraceToString(100, 2, "*\t")                         \
        << "-- -- -- -- -- -- -- --";                                          \
    assert(x);                                                                 \
  }

#define APEXSTORM_ASSERT2(x, w)                                                \
  if (APEXSTORM_UNLIKELY(!(x))) {                                              \
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())                                  \
        << "\n-- -- -- -- -- -- -- --\n"                                       \
        << "*  Assertion: " #x << "\n-- message:\n*  " << w                    \
        << "\n-- backtrace:\n"                                                 \
        << apexstorm::BacktraceToString(100, 2, "*\t")                         \
        << "-- -- -- -- -- -- -- --";                                          \
    assert(x);                                                                 \
  }

#endif
