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
    apexstorm::ByteArray::ptr ba(new apexstorm::ByteArray(1));                 \
    for (auto &i : vec) {                                                      \
      ba->write_fun(i);                                                        \
    }                                                                          \
    ba->setPosition(0);                                                        \
    for (size_t i = 0; i < vec.size(); ++i) {                                  \
      type v = ba->read_fun();                                                 \
      if (v != vec[i]) {                                                       \
        APEXSTORM_LOG_DEBUG(g_logger) << i << " : " << v << " - " << vec[i];   \
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
    apexstorm::ByteArray::ptr ba2(new apexstorm::ByteArray(2));                \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba2->readFromFile(path));                                 \
    ba->setPosition(0);                                                        \
    ba2->setPosition(0);                                                       \
    APEXSTORM_ASSERT(ba->toString() == ba2->toString());                       \
    APEXSTORM_ASSERT(ba->getPosition() == 0);                                  \
    APEXSTORM_ASSERT(ba2->getPosition() == 0);                                 \
    APEXSTORM_LOG_INFO(g_logger) << "(" #type ") write/read file finished";    \
  }

  XX(int8_t, 100, writeFint8, readFint8, 1)
  XX(uint8_t, 100, writeFuint8, readFuint8, 1)
  XX(int16_t, 100, writeFint16, readFint16, 1)
  XX(uint16_t, 100, writeFuint16, readFuint16, 1)
  XX(int32_t, 100, writeFint32, readFint32, 1)
  XX(uint32_t, 100, writeFuint32, readFuint32, 1)
  XX(int64_t, 100, writeFint64, readFint64, 1)
  XX(uint64_t, 100, writeFuint64, readFuint64, 1)

  XX(int32_t, 100, writeInt32, readInt32, 1)
  XX(uint32_t, 100, writeUint32, readUint32, 1)
  XX(int64_t, 100, writeInt64, readInt64, 1)
  XX(uint64_t, 100, writeUint64, readUint64, 1)

  XX(float, 100, writeFloat, readFloat, 1)
  XX(double, 100, writeDouble, readDouble, 1)
#undef XX
}

std::string generate_random_string(int min_len, int max_len) {
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

void test_string() {}

int main() {
  test_basic();
  return 0;
}