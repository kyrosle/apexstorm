/**
 * @file socket.h
 * @brief Socket encapsulation
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_SOCKET_H__
#define __APEXSTORM_SOCKET_H__

#include "address.h"
#include "noncopyable.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <sys/socket.h>

namespace apexstorm {

// Socket
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
  typedef std::shared_ptr<Socket> ptr;
  typedef std::weak_ptr<Socket> weak_ptr;

  // Socket type
  enum class Type {
    // TCP type
    TCP = SOCK_STREAM,
    // UDP type
    UDP = SOCK_DGRAM,
  };

  // Socket protocol cluster
  enum class Family {
    // Ipv4 socket
    IPv4 = AF_INET,
    // Ipv6 socket
    IPv6 = AF_INET6,
    // unix socket
    Unix = AF_UNIX,
  };

  // Create TCP socket through Address
  static Socket::ptr CreateTCP(apexstorm::Address::ptr address);
  // Create UDP socket through Address
  static Socket::ptr CreateUDP(apexstorm::Address::ptr address);

  // Create TCP Ipv4 socket
  static Socket::ptr CreateTCPSocket();
  // Create UDP Ipv4 socket
  static Socket::ptr CreateUDPSocket();

  // Create TCP Ipv6 socket
  static Socket::ptr CreateTCPSocket6();
  // Create UDP Ipv6 socket
  static Socket::ptr CreateUDPSocket6();

  // Create Unix TCP socket
  static Socket::ptr CreateUnixTCPSocket();
  // Create Unix UDP socket
  static Socket::ptr CreateUnixUDPSocket();

  // Constructor
  // @param: family   - ipv4/ipv6/unix
  // @param: type     - tcp/udp
  // @param: protocol - protocol
  Socket(int family, int type, int protocol = 0);
  Socket(int family, Type type, int protocol = 0)
      : Socket(family, (int)type, protocol) {}
  Socket(Family family, Type type, int protocol = 0)
      : Socket((int)family, (int)type, protocol) {}

  // Destructor
  ~Socket();

  // Get the send timeout, from fd manager.
  int64_t getSendTimeout();
  // Set the send timeout
  void setSendTimeout(int64_t timeout);

  // Get the receive timeout, from fd manager.
  int64_t getRecvTimeout();
  // Set the receive timeout
  void setRecvTimeout(int64_t timeout);

  // Get socket options
  bool getOption(int level, int option, void *result, size_t *len);
  template <class T> bool getOption(int level, int option, T &result) {
    return getOption(level, option, &result, sizeof(T));
  }

  // Set socket options
  bool setOption(int level, int option, const void *value, size_t len);
  template <class T> bool setOption(int level, int option, T &value) {
    return setOption(level, option, &value, sizeof(T));
  }

  // Receive connection.
  // If succeed, return socket, otherwise return nullptr.
  // The socket should be `bind`, `listen` successfully.
  Socket::ptr accept();

  // Bind address
  // Return whether binding successfully.
  bool bind(const Address::ptr addr);

  // Connect the address
  // @param: timeout_ms - if not setting, it will use the configured timeout.
  // Return whether connecting successfully.
  bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

  // Listening the socket.
  // @param: backlog - Maximum length of the pending connection queue.
  // Return whether listening successfully.
  bool listen(int backlog = SOMAXCONN);

  // Close the socket
  bool close();

  // Send data
  // @param: buffer - memory for data to be sent
  // @param: length - the length of the data to be sent
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int send(const void *buffer, size_t length, int flags = 0);

  // Send data
  // @param: buffer - memory for data to be sent(iovec array)
  // @param: length - the length of the data to be sent(iovec array length)
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int send(const iovec *buffers, size_t length, int flags = 0);

  // Send data
  // @param: buffer - memory for data to be sent
  // @param: length - the length of the data to be sent
  // @param: to     - destination address
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int sendTo(const void *buffer, size_t length, const Address::ptr to,
             int flags = 0);

  // Send data
  // @param: buffer - memory for data to be sent(iovec array)
  // @param: length - the length of the data to be sent(iovec array length)
  // @param: to     - destination address
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int sendTo(const iovec *buffers, size_t length, const Address::ptr to,
             int flags = 0);

  // Receive data
  // @param: buffer - memory for data to be received
  // @param: length - the length of the data to be received
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int recv(void *buffer, size_t length, int flags = 0);

  // Receive data
  // @param: buffer - memory for data to be received(iovec array)
  // @param: length - the length of the data to be received(iovec array length)
  // @param: to     - destination address
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int recv(iovec *buffers, size_t length, int flags = 0);

  // Receive data
  // @param: buffer - memory for data to be received
  // @param: length - the length of the data to be received
  // @param: to     - destination address
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int recvFrom(void *buffer, size_t length, const Address::ptr from,
               int flags = 0);

  // Receive data
  // @param: buffer - memory for data to be received(iovec array)
  // @param: length - the length of the data to be received(iovec array length)
  // @Return value:
  // > 0: the data of the corresponding size was successfully sent
  // = 0: socket was closed
  // < 0: socket meets error
  int recvFrom(iovec *buffers, size_t length, const Address::ptr from,
               int flags = 0);

  // Get remote address.
  Address::ptr getRemoteAddress();
  // Get local address.
  Address::ptr getLocalAddress();

  // Get protocol cluster.
  int getFamily() const { return m_family; }

  // Get socket type.
  int getType() const { return m_type; }

  // Get protocol.
  int getProtocol() const { return m_protocol; }

  // Return whether the socket is connected.
  bool isConnected() const { return m_isConnected; }

  // Check the socket whether is available.
  bool isValid() const;

  // Return socket error.
  int getError();

  // Ouput the information to stream.
  std::ostream &dump(std::ostream &os) const;

  // Get the socket file descriptor
  int getSocket() const { return m_sock; }

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

protected:
  // Initialize the socket.
  void initSock();

  // Create the socket.
  void newSock();

  // Initialize the corresponding socket.
  bool init(int sock);

private:
  // socket file descriptor.
  int m_sock;
  // socket protocol descriptor.
  int m_family;
  // socket type.
  int m_type;
  // socket protocol.
  int m_protocol;
  // socket connected status.
  bool m_isConnected;
  // socket local address.
  Address::ptr m_localAddress;
  // socket remote address.
  Address::ptr m_remoteAddress;
};

} // namespace apexstorm

#endif