#ifndef __APEXSTORM_SINGLETON_H__
#define __APEXSTORM_SINGLETON_H__

#include <memory>

namespace apexstorm {

/**
 * @brief Singleton pattern encapsulation class
 * @tparam T Type
 * @tparam X In order to create tags corresponding to multiple instances
 * @tparam N Create multiple instance indexes with the same Tag
 */
template <class T, class X = void, int N = 0> class Singleton {
public:
  /**
   * @brief Return singleton raw pointer
   */
  static T *GetInstance() {
    static T v;
    return &v;
  }
};

/**
 * @brief Singleton pattern smart pointer encapsulation class
 * @tparam T Type
 * @tparam X In order to create tags corresponding to multiple instances
 * @tparam N Create multiple instance indexes with the same Tag
 */
template <class T, class X = void, int N = 0> class SingletonPtr {
public:
  /**
   * @brief Return singleton smart pointer
   */
  static std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};

} // namespace apexstorm

#endif