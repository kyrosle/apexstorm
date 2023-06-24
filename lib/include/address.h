/**
 * @file address.h
 * @brief Encapsulation of network addresses(ipv4, ipv6 and unix)
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_ADDRESS_H__
#define __APEXSTORM_ADDRESS_H__

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <utility>
#include <vector>

namespace apexstorm {

class IPAddress;

template <class T> static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;
}

// count the number of 1 in value
template <class T> static uint32_t CountBytes(T value) {
  uint32_t result = 0;
  for (; value; ++result) {
    value &= value - 1;
  }
  return result;
}

// Network address abstraction.
class Address {
public:
  typedef std::shared_ptr<Address> ptr;

  // Create a network address through a (sockaddr) pointer.
  // @param: addr    - sockaddr pointer
  // @param: addrlen - length of sockaddr
  // Return a address reference of sockaddr otherwise return a nullptr
  static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

  // Return search all corresponding `Address`s through the `host`.
  //  @param: host        - domain name, server ip address ...
  //  @param: family      - AF_INET(IPv4), AF_INET6(IPv6), AF_UNSPEC(IPv4 and
  //  IPv6), AF_UNIX(unix)
  //  @param: socket type - SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, 0(all)
  //  @param: protocol    - IPPROTO_TCP, IPPROTO_UDP
  // Return whether it is succeed, and the address are stored in the `result`
  // vector.
  static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family = AF_INET, int type = 0, int protocol = 0);

  // Return search any corresponding `Address` through the `host`.
  //  @param: host        - domain name, server ip address ...
  //  @param: family      - AF_INET(IPv4), AF_INET6(IPv6), AF_UNSPEC(IPv4 and
  //  IPv6), AF_UNIX(unix)
  //  @param: socket type - SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, 0(all)
  //  @param: protocol    - IPPROTO_TCP, IPPROTO_UDP
  // Return any corresponding address, otherwise return nullptr.
  static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                                int type = 0, int protocol = 0);

  // Return search any corresponding `IPAddress` through the `host`.
  //  @param: host        - domain name, server ip address ...
  //  @param: family      - AF_INET(IPv4), AF_INET6(IPv6), AF_UNSPEC(IPv4 and
  //  IPv6), AF_UNIX(unix)
  //  @param: socket type - SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, 0(all)
  //  @param: protocol    - IPPROTO_TCP, IPPROTO_UDP
  // Return any corresponding ipaddress, otherwise return nullptr.
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
                                                       int family = AF_UNSPEC,
                                                       int type = 0,
                                                       int protocol = 0);

  // Return all network interface cards of the machine.
  // @param[out]: result - type of multimap <interface name, (address, netmask
  // bit)>
  // @param: family      - specified protocol cluster (AF_INET(IPv4),
  // AF_INET6(IPv6), AF_UNSPEC(IPv4 and IPv6), AF_UNIX(unix))
  // Return whether it is succeed, and store the interfaces information in
  // `result`.
  static bool GetInterFaceAddresses(
      std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
      int family = AF_INET);

  // Gets the address and number of subnetmask bits of the specified network
  // interface card.
  // @param: result - type of <(Address, netmask bit)>
  // @param: iface  - the name of the interface
  // @param: family - specified protocol cluster (AF_INET(IPv4), AF_INET6(IPv6),
  // AF_UNSPEC(IPv4 and IPv6), AF_UNIX(unix))
  // Return whether it is succeed, and store the interface information in
  // `result`.
  static bool
  GetInterFaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                        const std::string &iface, int family = AF_UNSPEC);

  // Destructor
  virtual ~Address() {}

  // Return the protocol cluster
  int getFamily() const;

  // Return the pointer of sockaddr(only readable).
  virtual const sockaddr *getAddr() const = 0;

  // Return the pointer of sockaddr.
  virtual sockaddr *getAddr() = 0;

  // Get the sockaddr length.
  virtual socklen_t getAddrLen() const = 0;

  // Readable output address.
  virtual std::ostream &insert(std::ostream &os) const = 0;

  // Return readable string.
  std::string toString();

  bool operator<(const Address &rhs) const;
  bool operator==(const Address &rhs) const;
  bool operator!=(const Address &rhs) const;
};

// The abstraction of Ip address.
class IPAddress : public Address {
public:
  typedef std::shared_ptr<IPAddress> ptr;

  // Create a IPAddress through domain name, server ip.
  // @param: address - domain name, server ip
  // @param: port    - port number
  // Return IPAddress if successfully, otherwise return nullptr.
  static IPAddress::ptr Create(const char *address, uint16_t port = 0);

  // Get the broadcast address of this ip address.
  // @param: prefix_len - the length of the netmask.
  // Return IPAddress of broadcast address if successfully, otherwise return
  // nullptr.
  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

  // Get the network segment address of this ip address.
  // @param: prefix_len - the length of the netmask.
  // Return IPAddress of broadcast address if successfully, otherwise return
  // nullptr.
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

  // Get the network mask address of this ip address.
  // @param: prefix_len - the length of the netmask.
  // Return IPAddress of broadcast address if successfully, otherwise return
  // nullptr.
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  // Get the network port number.
  virtual uint32_t getPort() const = 0;

  // Set the network port number.
  virtual void setPort(uint16_t v) = 0;
};

// IPv4
class IPv4Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv4Address> ptr;

  // Create IPv4 address through `sockaddr_in` structure.
  IPv4Address(const sockaddr_in &address) : m_addr(address) {}

  // Create IPv4 address through binary address.
  // default address is 0.0.0.0
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  // Create a new IPv4 address through dotted decimal address.
  // @param: address - dotted decimal address, suck as 192.168.0.1
  // @param: port    - port number
  // Return Ipv4Address if successfully created, otherwise return nullptr.
  static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  uint32_t getPort() const override;
  void setPort(uint16_t v) override;

private:
  sockaddr_in m_addr;
};

// IPv6
class IPv6Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv6Address> ptr;

  // Default constructor
  IPv6Address();

  // Create a Ipv6 address through `sockaddr_in` structure.
  IPv6Address(const sockaddr_in6 &address) : m_addr(address) {}

  // Create a Ipv6 address through binary address
  IPv6Address(const char *address, uint16_t port = 0);

  // Create a Ipv6 address through a address string.
  // @param: address - address string
  // @param: port    - port number
  static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  uint32_t getPort() const override;
  void setPort(uint16_t v) override;

private:
  sockaddr_in6 m_addr;
};

// Unix
class UnixAddress : public Address {
public:
  typedef std::shared_ptr<UnixAddress> ptr;

  // Default constructor
  UnixAddress();

  // Create a unix address through path.
  // @param: path - UnixSocket path(length not over UNIX_PATH_MAX)
  UnixAddress(const std::string &path);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  void setAddrLen(uint32_t len);
  std::ostream &insert(std::ostream &os) const override;

private:
  struct sockaddr_un m_addr;
  socklen_t m_length;
};

// Unknown
class UnknownAddress : public Address {
public:
  typedef std::shared_ptr<UnknownAddress> ptr;
  UnknownAddress(int family);
  UnknownAddress(const sockaddr &addr) : m_addr(addr) {}

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

private:
  sockaddr m_addr;
};

inline std::ostream &operator<<(std::ostream &os, const Address &addr) {
  addr.insert(os);
  return os;
}
} // namespace apexstorm

#endif