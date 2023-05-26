#ifndef __APEXSTORM__UTIL_H__
#define __APEXSTORM__UTIL_H__

#include <cstdint>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace apexstorm {

/**
 * @brief Returns current thread id
 */
pid_t GetThreadId();

/**
 * @brief Returns current fiber id
 */
uint32_t GetFiberId();

} // namespace apexstorm
#endif