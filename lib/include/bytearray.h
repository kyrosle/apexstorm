#ifndef __APEXSTORM_BYTEARRAY_H__
#define __APEXSTORM_BYTEARRAY_H__

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

namespace apexstorm {

// Serializer and deserializer in byte level.
class ByteArray {
public:
  typedef std::shared_ptr<ByteArray> ptr;

  // byte array node
  struct Node {
    // constructor with node size
    Node(size_t s);
    // default constructor with 0 node size
    Node();
    // destructor
    ~Node();

    // current byte array pointer
    char *ptr;
    // next byte array node
    Node *next;
    // current byte array node size
    size_t size;
  };

  ByteArray(size_t base_size = 4096); /* default node size 4K */
  ~ByteArray();

  /* write base integer with fixed length(not compressed) */
  void writeFint8(int8_t value);
  void writeFuint8(uint8_t value);
  void writeFint16(int16_t value);
  void writeFuint16(uint16_t value);
  void writeFint32(int32_t value);
  void writeFuint32(uint32_t value);
  void writeFint64(int64_t value);
  void writeFuint64(uint64_t value);

  /* write base integer (compressed) */
  void writeInt32(int32_t value);
  void writeUint32(uint32_t value);
  void writeInt64(int64_t value);
  void writeUint64(uint64_t value);

  /* write float(32bit)/double(64bit) */
  void writeFloat(float value);
  void writeDouble(double value);

  // format: length(int16)data
  void writeStringF16(const std::string &value);
  // format: length(int32)data
  void writeStringF32(const std::string &value);
  // format: length(int64)data
  void writeStringF64(const std::string &value);
  // format: length(varint)data
  void writeStringVint(const std::string &value);
  // format: data
  void writeStringWithoutLength(const std::string &value);

  /* read base integer with fixed length(not compressed) */
  int8_t readFint8();
  uint8_t readFuint8();
  int16_t readFint16();
  uint16_t readFuint16();
  int32_t readFint32();
  uint32_t readFuint32();
  int64_t readFint64();
  uint64_t readFuint64();

  /* read base integer (compressed) */
  int32_t readInt32();
  uint32_t readUint32();
  int64_t readInt64();
  uint64_t readUint64();

  /* read float(32bit)/double(64bit) */
  float readFloat();
  double readDouble();

  // format: length(int16)data
  std::string readStringF16();
  // format: length(int32)data
  std::string readStringF32();
  // format: length(int64)data
  std::string readStringF64();
  // format: length(varint)data
  std::string readStringVint();

  // inside operation
  void clear();

  // Write value into bytearray(may advance the pos pointer)
  void write(const void *buf, size_t size);

  // Write value from bytearray(may advance the pos pointer)
  void read(void *buf, size_t size);

  // Write value from bytearray(will not change any member information, reading
  // value from the specified position)
  void read(void *buf, size_t size, size_t position) const;

  // Get the current position in the list of nodes.
  size_t getPosition() const { return m_position; }

  // Set the current position in the list of nodes.
  void setPosition(size_t pos);

  // Write the bytearray into the file(will not change any member information)
  // bytearray[pos..] -> file
  bool writeToFile(const std::string &name) const;

  // Read the bytearray from the file
  bool readFromFile(const std::string &name);

  // Get the base node size
  size_t getBaseSize() const { return m_baseSize; }

  // Get readable size of bytes.
  size_t getReadSize() const { return m_size - m_position; }

  // Return current platform whether is littleEndian sequence.
  bool isLittleEndian() const;

  // Set this bytearray whether is littleEndian sequence.
  void setIsLittleEndian(bool value);

  // Return the bytearray binary sequence.
  std::string toString() const;

  // Return the bytearray hex sequence.
  std::string toHexString() const;

  // Get the readable cache and save it as an iovec array,
  // Copy the bytearray[m_pos..] into vector of iovec,
  // only read, not changing m_position,
  // return the total read bytes size.
  uint64_t getReadBuffers(std::vector<iovec> &buffers,
                          uint64_t len = ~0ull) const;

  // Get the readable cache and save it as an iovec array,
  // Copy the bytearray[pos..] into vector of iovec,
  // it can read from any valid pointer from bytearray,
  // only read, not changing m_position,
  // return the total read bytes size.
  uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                          uint64_t position) const;

  // Get the writeable cache and save it as an iovec array,
  // Copy the bytearray[m_pos..] into vector of iovec,
  // only extend capacity,. not changing m_position.
  uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

  // Get the bytearray size.
  size_t getSize() const { return m_size; }

private:
  // Extend the bytearray capacity.
  void addCapacity(size_t size);

  // Get the bytearray capacity can be written in.
  size_t getCapacity() const { return m_capacity - m_position; }

private:
  // the size of node char array
  size_t m_baseSize;
  // the current position in the list of nodes
  size_t m_position;
  // the capacity of the list of nodes
  size_t m_capacity;
  // the size of the list of nodes
  size_t m_size;
  // the type of endian
  int8_t m_endian;

  // root node
  Node *m_root;
  // current node of m_position
  Node *m_cur;
};

} // namespace apexstorm

#endif
