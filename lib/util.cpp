#include "./include/util.h"

namespace apexstorm {

pid_t GetThreadId() { return syscall(SYS_gettid); }

uint32_t GetFiberId() { return 0; }

} // namespace apexstorm