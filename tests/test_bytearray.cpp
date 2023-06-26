#include "apexstorm.h"
#include "bytearray.h"
#include "log.h"
#include "macro.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_basic() {
  APEXSTORM_LOG_INFO(g_logger)
      << "[Transformation testing and write/read file testing...]";

#define XX(type, len, write_fun, read_fun, base_len)                           \
  {                                                                            \
    std::vector<type> vec;                                                     \
    for (int i = 0; i < len; ++i) {                                            \
      vec.push_back(rand());                                                   \
    }                                                                          \
    apexstorm::ByteArray::ptr ba(new apexstorm::ByteArray(base_len));          \
    for (auto &i : vec) {                                                      \
      ba->write_fun(i);                                                        \
    }                                                                          \
    ba->setPosition(0);                                                        \
    for (size_t i = 0; i < vec.size(); ++i) {                                  \
      type v = ba->read_fun();                                                 \
      if (v != vec[i]) {                                                       \
        APEXSTORM_LOG_DEBUG(g_logger)                                          \
            << i << " : read=" << v << " - actual=" << vec[i];                 \
      }                                                                        \
      APEXSTORM_ASSERT(v == vec[i]);                                           \
    }                                                                          \
    APEXSTORM_ASSERT(ba->getReadSize() == 0);                                  \
    APEXSTORM_LOG_INFO(g_logger)                                               \
        << #write_fun "/" #read_fun " (" #type ") len=" << len                 \
        << " base_len=" << base_len << " size=" << ba->getSize();              \
    APEXSTORM_ASSERT(ba->getReadSize() == 0);                                  \
    std::string path = "/tmp/" #type "_" #len ".dat";                          \
    ba->setPosition(0);                                                        \
    APEXSTORM_ASSERT(ba->writeToFile(path));                                   \
    apexstorm::ByteArray::ptr ba2(new apexstorm::ByteArray(base_len * 2));     \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba2->readFromFile(path));                                 \
    ba->setPosition(0);                                                        \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba->toString() == ba2->toString());                       \
    APEXSTORM_ASSERT(ba->getPosition() == 0);                                  \
    APEXSTORM_ASSERT(ba2->getPosition() == 0);                                 \
    APEXSTORM_LOG_INFO(g_logger) << "(" #type ") write/read file finished";    \
  }

  for (int len = 1; len <= 20; ++len) {
    for (int base_size = 1; base_size <= 20; base_size += len) {
      XX(int8_t, len, writeFint8, readFint8, base_size)
      XX(uint8_t, len, writeFuint8, readFuint8, base_size)
      XX(int16_t, len, writeFint16, readFint16, base_size)
      XX(uint16_t, len, writeFuint16, readFuint16, base_size)
      XX(int32_t, len, writeFint32, readFint32, base_size)
      XX(uint32_t, len, writeFuint32, readFuint32, base_size)
      XX(int64_t, len, writeFint64, readFint64, base_size)
      XX(uint64_t, len, writeFuint64, readFuint64, base_size)

      XX(int32_t, len, writeInt32, readInt32, base_size)
      XX(uint32_t, len, writeUint32, readUint32, base_size)
      XX(int64_t, len, writeInt64, readInt64, base_size)
      XX(uint64_t, len, writeUint64, readUint64, base_size)

      XX(float, len, writeFloat, readFloat, base_size)
      XX(double, len, writeDouble, readDouble, base_size)
    }
  }

#undef XX
}

// generate a random string, using for testing relevant string serializer and
// deserializer.
std::string generate_random_string(int min_len, int max_len) {
  // uniformly distributed to generate random number
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis('a', 'z');

  int len = std::uniform_int_distribution<>(min_len, max_len)(gen);
  std::string str(len, '\0');
  for (int i = 0; i < len; i++) {
    str[i] = dis(gen);
  }
  return str;
}

void test_string() {
#define XX(type, len, write_fun, read_fun, base_len, min_len, max_len)         \
  {                                                                            \
    std::vector<std::string> vec;                                              \
    for (int i = 0; i < len; ++i) {                                            \
      vec.push_back(generate_random_string(min_len, max_len));                 \
    }                                                                          \
    apexstorm::ByteArray::ptr ba(new apexstorm::ByteArray(base_len));          \
    for (auto &i : vec) {                                                      \
      ba->write_fun(i);                                                        \
    }                                                                          \
    ba->setPosition(0);                                                        \
    for (int i = 0; i < len; ++i) {                                            \
      std::string str = ba->read_fun();                                        \
      APEXSTORM_ASSERT(str == vec[i]);                                         \
    }                                                                          \
    APEXSTORM_ASSERT(ba->getReadSize() == 0);                                  \
    APEXSTORM_LOG_INFO(g_logger)                                               \
        << #write_fun "/" #read_fun " (std::string " #type ") len=" << len     \
        << " base_len=" << base_len << " size=" << ba->getSize();              \
    APEXSTORM_ASSERT(ba->getReadSize() == 0);                                  \
    std::string path = "/tmp/std_string" #type "_" #len ".dat";                \
    ba->setPosition(0);                                                        \
    APEXSTORM_ASSERT(ba->writeToFile(path));                                   \
    apexstorm::ByteArray::ptr ba2(new apexstorm::ByteArray(base_len * 2));     \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba2->readFromFile(path));                                 \
    ba->setPosition(0);                                                        \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba->toString() == ba2->toString());                       \
    APEXSTORM_ASSERT(ba->getPosition() == 0);                                  \
    APEXSTORM_ASSERT(ba2->getPosition() == 0);                                 \
    APEXSTORM_LOG_INFO(g_logger)                                               \
        << "(std::string " #type ") write/read file finished";                 \
  }
  for (int len = 1; len <= 20; ++len) {
    for (int base_size = 1; base_size <= 20; base_size += len) {
      XX(f16, len, writeStringF16, readStringF16, base_size, 0, 20);
      XX(f32, len, writeStringF32, readStringF32, base_size, 0, 20);
      XX(f64, len, writeStringF64, readStringF64, base_size, 0, 20);
      XX(varint, len, writeStringVint, readStringVint, base_size, 0, 20);
    }
  }

  XX(f16, 100, writeStringF16, readStringF16, 1, 0, 10);
  XX(f32, 100, writeStringF32, readStringF32, 1, 0, 10);
  XX(f64, 100, writeStringF64, readStringF64, 1, 0, 10);
  XX(varint, 100, writeStringVint, readStringVint, 1, 0, 10);
#undef XX
}

int main() {
  // test_basic();
  test_string();
  return 0;
}