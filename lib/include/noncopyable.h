#ifndef __APEXSTORM_NONCOPYABLE_H__
#define __APEXSTORM_NONCOPYABLE_H__

namespace apexstorm {

class Noncopyable {
public:
  Noncopyable() = default;
  ~Noncopyable() = default;
  Noncopyable(const Noncopyable &) = delete;
  Noncopyable &operator=(const Noncopyable &) = delete;
};

} // namespace apexstorm

#endif