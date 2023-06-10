/**
 * @file util.h
 * @brief utilities.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM__UTIL_H__
#define __APEXSTORM__UTIL_H__

#include <cstdint>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace apexstorm {

// Returns current thread id.
pid_t GetThreadId();

// Returns current fiber id.
uint32_t GetFiberId();

void Backtrace(std::vector<std::string> &bt, int size, int skip);
std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string &prefix = "\t");

// Get time
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();

} // namespace apexstorm
#endif