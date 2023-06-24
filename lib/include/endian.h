#ifndef __APEXSTORM_ENDIAN_H__
#define __APEXSTORM_ENDIAN_H__

#define APEXSTORM_LITTLE_ENDIAN 1
#define APEXSTORM_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace apexstorm {

extern "C++" template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
  return (T)bswap_64((uint64_t)value);
}
extern "C++" template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
  return (T)bswap_32((uint32_t)value);
}
extern "C++" template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
  return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define APEXSTORM_BYTE_ORDER APEXSTORM_BIG_ENDIAN
#else
#define APEXSTORM_BYTE_ORDER APEXSTORM_LITTER_ENDIAN
#endif

#if APEXSTORM_BYTE_ORDER == APEXSTORM_BIG_ENDIAN
extern "C++" template <class T> T byteswapOnLittleEndian(T t) {
  return byteswap(t);
}
extern "C++" template <class T> T byteswapOnBigEndian(T t) { return t; }
#else
extern "C++" template <class T> T byteswapOnLittleEndian(T t) { return t; }
extern "C++" template <class T> T byteswapOnBigEndian(T t) {
  return byteswap(t);
}
#endif
} // namespace apexstorm

#endif