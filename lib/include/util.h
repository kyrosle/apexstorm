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
#include <type_traits>
#include <unistd.h>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif
#include <cstdlib>
#include <iostream>

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

namespace debug_utils {

template <class T> std::string cpp_type_name() {
  const char *name = typeid(T).name();
#if defined(__GNUC__) || defined(__clang__)
  int status;
  char *p = abi::__cxa_demangle(name, 0, 0, &status);
  std::string s = p;
  std::free(p);
#else
  std::string s = name;
#endif
  if (std::is_const<typename std::remove_reference<T>::type>::value) {
    s += " const";
  }
  if (std::is_volatile<typename std::remove_reference<T>::type>::value) {
    s += " volatile";
  }
  if (std::is_lvalue_reference<T>::value) {
    s += " &";
  }
  if (std::is_rvalue_reference<T>::value) {
    s += " &&";
  }
  return s;
}

template <class T> std::string cpp_value_type_name(T value) {
  return cpp_type_name<T>();
}

} // namespace debug_utils
#endif