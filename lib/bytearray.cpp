#include "bytearray.h"
#include "log.h"
#include "util.h"
#include <bitset>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

ByteArray::Node::Node(size_t s) : ptr(new char[s]), size(s), next(nullptr) {}

ByteArray::Node::Node() : ptr(nullptr), size(0), next(nullptr) {}

ByteArray::Node::~Node() {
  // release the char array
  if (ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0),
      m_root(new Node(base_size)), m_cur(m_root),
      m_endian(APEXSTORM_BIG_ENDIAN) /* default endian is big-endian */ {}

ByteArray::~ByteArray() {
  Node *tmp = m_root;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
}

bool ByteArray::isLittleEndian() const {
  return m_endian == APEXSTORM_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool value) {
  if (value) {
    m_endian = APEXSTORM_LITTLE_ENDIAN;
  } else {
    m_endian = APEXSTORM_BIG_ENDIAN;
  }
}

/* --------- encode ---------*/

void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }
void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }
/* i16, u16, i32, u32, i64, u64 considering endian */
void ByteArray::writeFint16(int16_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}
void ByteArray::writeFuint16(uint16_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}
void ByteArray::writeFint32(int32_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}
void ByteArray::writeFuint32(uint32_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}
void ByteArray::writeFint64(int64_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}
void ByteArray::writeFuint64(uint64_t value) {
  if (m_endian != APEXSTORM_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

// template method, zigzag encode the negative/positive values
// I: int32_t -> R: uint32_t
//    int64_t       uint64_t
template <class R, class I> static R EncodeZigzag(const I &v) {
  if (v < 0) {
    return ((R)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

// template method, zigzag decode the negative/positive values
// I: int32_t -> R: uint32_t
//    int64_t       uint64_t
template <class R, class I> static R DecodeZigzag(const I &v) {
  return (v >> 1) ^ -(v & 1);
}

/*
  Compress i32, u32, i64, u64 intergers
*/
void ByteArray::writeInt32(int32_t value) {
  writeUint32(EncodeZigzag<uint32_t>(value));
}

void ByteArray::writeUint32(uint32_t value) {
  uint8_t tmp[5]; /* 4 byte 32 bit */
  // compression
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

void ByteArray::writeInt64(int64_t value) {
  writeUint64(EncodeZigzag<uint64_t>(value));
}
void ByteArray::writeUint64(uint64_t value) {
  uint8_t tmp[10]; /* 8 byte 64 bit */
  // compression
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

/* transform float/double into uint32_t/uint64_t(memcpy), bitset is different
 * but it works for each trans */
void ByteArray::writeFloat(float value) {
  uint32_t v;
  memcpy(&v, &value, sizeof(value));

  // TODO: support zigzag encoding ?
  writeFuint32(v);
}
void ByteArray::writeDouble(double value) {
  uint64_t v;
  memcpy(&v, &value, sizeof(value));
  // TODO: support zigzag encoding ?
  writeFuint64(v);
}

/* write string with length or raw string */
void ByteArray::writeStringF16(const std::string &value) {
  writeFuint16(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringF32(const std::string &value) {
  writeFuint32(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringF64(const std::string &value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringVint(const std::string &value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringWithoutLength(const std::string &value) {
  write(value.c_str(), value.size());
}

/* --------- decode ---------*/

int8_t ByteArray::readFint8() {
  int8_t v;
  read(&v, sizeof(v));
  return v;
}
uint8_t ByteArray::readFuint8() {
  uint8_t v;
  read(&v, sizeof(v));
  return v;
}

#define XX(type)                                                               \
  type v;                                                                      \
  read(&v, sizeof(v));                                                         \
  if (m_endian == APEXSTORM_BYTE_ORDER) {                                      \
    return v;                                                                  \
  } else {                                                                     \
    return byteswap(v);                                                        \
  }

int16_t ByteArray::readFint16() { XX(int16_t); }
uint16_t ByteArray::readFuint16() { XX(uint16_t); }
int32_t ByteArray::readFint32() { XX(int32_t); }
uint32_t ByteArray::readFuint32() { XX(uint32_t); }
int64_t ByteArray::readFint64() { XX(int64_t); }
uint64_t ByteArray::readFuint64() { XX(uint64_t); }

#undef XX

int32_t ByteArray::readInt32() { return DecodeZigzag<int32_t>(readUint32()); }
uint32_t ByteArray::readUint32() {
  uint32_t result = 0;
  // decompression
  for (uint8_t i = 0; i < 32; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint32_t)b) << i;
      break;
    } else {
      result |= (((uint32_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

int64_t ByteArray::readInt64() { return DecodeZigzag<int64_t>(readUint64()); }
uint64_t ByteArray::readUint64() {
  uint64_t result = 0;
  // decompression
  for (uint8_t i = 0; i < 64; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint64_t)b) << i;
      break;
    } else {
      result |= (((uint64_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

float ByteArray::readFloat() {
  // TODO: support zigzag encoding ?
  uint32_t v = readFuint32();
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}
double ByteArray::readDouble() {
  uint64_t v = readFuint64();
  double value;
  memcpy(&value, &v, sizeof(value));
  // TODO: support zigzag encoding ?
  return value;
}

std::string ByteArray::readStringF16() {
  uint16_t len = readFuint16();
  std::string buff;
  buff.resize(len);
  if (!buff.empty()) {
    read(&buff[0], len);
  }
  return buff;
}
std::string ByteArray::readStringF32() {
  uint32_t len = readFuint32();
  std::string buff;
  buff.resize(len);
  if (!buff.empty()) {
    read(&buff[0], len);
  }
  return buff;
}
std::string ByteArray::readStringF64() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  if (!buff.empty()) {
    read(&buff[0], len);
  }
  return buff;
}
std::string ByteArray::readStringVint() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  if (!buff.empty()) {
    read(&buff[0], len);
  }
  return buff;
}

/* --------- member method ---------*/

void ByteArray::clear() {
  m_position = m_size = 0;
  m_capacity = m_baseSize;
  // release node recursion
  Node *tmp = m_root->next;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = NULL;
}

void ByteArray::write(const void *buf, size_t size) {
  if (size == 0) {
    return;
  }
  // extend the capacity
  addCapacity(size);

  // the node offset pos
  size_t npos = m_position % m_baseSize;
  // the capacity in corresponding pos node
  size_t ncap = m_cur->size - npos;
  // buffer pos
  size_t bpos = 0;

  // while for all buffer are written in
  while (size > 0) {
    if (ncap >= size) {
      // the capacity in corresponding pos node can
      // afford the buff(the last of len)
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);

      if (m_cur->size == (npos + size)) {
        // the current node of capacity is filled,
        // advance the node
        m_cur = m_cur->next;
      }
      // advance the pos pointer
      m_position += size;
      // advance the buffer pos pointer
      bpos += size;
      // clear buffer size
      size = 0;
    } else {
      // memcpy the acceptable len of buffer into the current node
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
      // advance current bytearray pos pointer
      m_position += ncap;
      // advance the buffer pos pointer
      bpos += ncap;
      // reduce the written len
      size -= ncap;
      // advance the node list
      m_cur = m_cur->next;
      // reset the acceptable size
      ncap = m_cur->size;
      // reset the node offset
      npos = 0;
    }
  }

  // fix the size of bytearray mark
  if (m_position > m_size) {
    m_size = m_position;
  }
}

void ByteArray::read(void *buf, size_t size) {
  // simillar logic with `ByteArray::write(const void*buf, size_t size)`
  if (size > getReadSize()) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "ByteArray::read(void*buf, size_t size) throw exception!";
    APEXSTORM_LOG_ERROR(g_logger) << "size=" << size << " m_size=" << m_size
                                  << " m_position=" << m_position;
    throw std::out_of_range("not enough len");
  }

  // offset in node
  size_t npos = m_position % m_baseSize;
  // acceptable capacity in node
  size_t ncap = m_cur->size - npos;
  // buffer pos pointer
  size_t bpos = 0;

  while (size > 0) {
    if (ncap >= size) {
      // exactly current node can accept the `size` of buffer
      memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
      if (m_cur->size == (npos + size)) {
        // node capacity is full
        m_cur = m_cur->next;
      }
      // advance bytearray pos pointer
      m_position += size;
      // advance buffer pos pointer
      bpos += size;
      // value is all read
      size = 0;
    } else {
      // only read `ncap` byte from bytearray
      memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
      // advance bytearray pos pointer
      m_position += ncap;
      // advance buffer pos pointer
      bpos += ncap;
      // reduce the bytes have been read
      size -= ncap;
      // advance node list
      m_cur = m_cur->next;
      // reset the acceptable byte in current node
      ncap = m_cur->size;
      npos = 0;
    }
  }
}

void ByteArray::read(void *buf, size_t size, size_t position) const {
  if (size > (m_size - position)) {
    APEXSTORM_LOG_ERROR(g_logger) << "ByteArray::read (void* buf, size_t size, "
                                     "size_t position) throw exception!";
    APEXSTORM_LOG_ERROR(g_logger)
        << "size=" << size << " m_size=" << m_size << " position=" << position;
    throw std::out_of_range("not enough len");
  }

  size_t npos = position /* specified position */ % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  Node *cur = m_cur; /* search the correspond node from node list */

  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, cur->ptr /* specified node */ + npos, size);
      if (cur->size /* specified node */ == (npos + size)) {
        cur = cur->next; /* specified node */
      }
      position += size; /* specified position */
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, cur->ptr /* specified node */ + npos, ncap);
      position += ncap; /* specified position */
      bpos += ncap;
      size -= ncap;
      cur = cur->next; /* specified node */
      ncap = cur->size;
      npos = 0;
    }
  }
}

void ByteArray::setPosition(size_t v) {
  // usually to reset the bytearray pointer position, for another operations
  if (v > m_capacity) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "ByteArray::setPosition(size_t v) throw exception!";
    APEXSTORM_LOG_ERROR(g_logger) << "v=" << v << " m_capacity=" << m_capacity;
    throw std::out_of_range("set_position out of range");
  }
  m_position = v;
  if (m_position > m_size) {
    // position is over the bytearray size
    m_size = m_position;
  }
  // recalculate the node pointer
  m_cur = m_root;
  while (v > m_cur->size) {
    v -= m_cur->size;
    m_cur = m_cur->next;
  }
  if (v == m_cur->size) {
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string &name) const {
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "writeToFile name=" << name << " error, errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }

  int64_t read_size = getReadSize();
  int64_t pos = m_position;
  Node *cur = m_cur;

  // iterate over the whole bytearray(list of nodes)
  while (read_size > 0) {
    // calculate the offset of the current node
    int diff = pos % m_baseSize;
    // size of byte written in
    int64_t len =
        ((read_size > (int64_t)m_baseSize) ? m_baseSize : read_size) - diff;

    ofs.write(cur->ptr + diff, len);
    // advance the node list
    cur = cur->next;
    // advance the temporaily bytearray pointer position
    pos += len;
    // reduce the the theory length write in
    read_size -= len;
  }
  return true;
}

bool ByteArray::readFromFile(const std::string &name) {
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);
  if (!ifs) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "readFromFile name=" << name << " error, errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }

  // buffer manager by shared_ptr
  std::shared_ptr<char> buff(new char[m_baseSize],
                             [](char *ptr) { delete[] ptr; });
  while (!ifs.eof()) {
    // read take the size of byte from file to fill into a node
    ifs.read(buff.get(), m_baseSize);
    // APEXSTORM_LOG_DEBUG(g_logger)
    //     << "read from buff=" << buff.get() << ", size=" << ifs.gcount();
    write(buff.get(), ifs.gcount());
  }
  return true;
}

void ByteArray::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  size_t old_cap = getCapacity();
  if (old_cap >= size) {
    // don't  have to add capacity
    return;
  }

  // calculate the size we should extend
  size = size - old_cap;
  // round the node we should extend
  size_t count = std::ceil(1.0 * size / m_baseSize);
  // (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);

  // root node
  Node *tmp = m_root;
  // locate the tail of nodes list
  while (tmp->next) {
    tmp = tmp->next;
  }

  // used for mark the last node should start to fill data,
  // this mark will use when the there is no capacity before extending
  Node *first = NULL;

  // extend the node list by new
  for (size_t i = 0; i < count; ++i) {
    tmp->next = new Node(m_baseSize);
    if (first == NULL) {
      first = tmp->next;
    }
    tmp = tmp->next;
    m_capacity += m_baseSize;
  }

  if (old_cap == 0) {
    m_cur = first;
  }
}

std::string ByteArray::toString() const {
  std::string str;
  str.resize(getReadSize());
  if (str.empty()) {
    return str;
  }
  // APEXSTORM_LOG_DEBUG(g_logger) << "toString, read size=" << getReadSize()
  //                               << " m_position=" << m_position;
  read(&str[0], str.size(), m_position);
  return str;
}

std::string ByteArray::toHexString() const {
  std::string str = toString();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i]
       << " ";
  }

  return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
  // theoretically size to fill into the buffers
  len = std::min(getReadSize(), len);
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  // offset in current node
  size_t npos = m_position % m_baseSize;
  // acceptable capacity in current node
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  // current node
  Node *cur = m_cur;

  while (len > 0) {
    if (ncap >= len) {
      // data, char array
      iov.iov_base = cur->ptr + npos;
      // length of char array
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base =
          cur->ptr +
          npos /* after handle the offset in start, this may become zero */;
      iov.iov_len = ncap;
      len -= ncap;
      // advance node list
      cur = cur->next;
      // reset the acceptable capacity of node
      ncap = cur->size;
      // reset the node offset
      npos = 0;
    }
    // append to buffers
    buffers.push_back(iov);
  }
  return size;
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
  // similar to `ByteArray::getReadBuffers(std::vector<>)`
  len = std::min(getReadSize(), len);
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position /* specified position */ % m_baseSize;

  size_t count = position /* specified position */ / m_baseSize;

  // locate the specified position
  Node *cur = m_root;
  while (count > 0) {
    cur = cur->next;
    --count;
  }

  size_t ncap = m_cur->size - npos;
  struct iovec iov;

  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }

  addCapacity(len);

  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;

  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }

    buffers.push_back(iov);
  }
  return size;
}

} // namespace apexstorm