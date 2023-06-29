#include "address.h"
#include "endian.h"
#include "log.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ifaddrs.h>
#include <map>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol)) {
    return result[0];
  }
  return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {
  std::vector<Address::ptr> addrs;
  if (Lookup(addrs, host, family, type, protocol)) {
    IPAddress::ptr addr;
    for (auto &i : addrs) {
      // APEXSTORM_LOG_DEBUG(g_logger) << i->toString();
      addr = std::dynamic_pointer_cast<IPAddress>(i);
      if (addr) {
        return addr;
      }
    }
  }
  return nullptr;
}

// TODO: the part of ipv6 should do unit testing
// Currently only support ipv4 and (ipv6)
bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {
  // parsing hostname

  // addrinfo <netdb.h>
  addrinfo hints, *results, *next;

  hints.ai_flags = 0;           /* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
  hints.ai_family = family;     /* AF_INET(IPv4) AF_INET6(IPv6)
                                      AF_UNSPEC(unspecified) */
  hints.ai_socktype = type;     /* SOCK_STREAM, SOCK_DGRAM */
  hints.ai_protocol = protocol; /* IPPROTO_IPV4(IPV6, UNIX, TCP ,UDP) */
  // other fields set 0 or null
  hints.ai_addrlen = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  std::string node;
  const char *service = NULL;

  // - memchr(
  //    s: pointer to the memory block to look for
  //    c: the character wanted to find
  //    n: the size of the memory block
  // ) return the pointer of the found character, otherwise return NULL

  // - substr(
  //    pos: the first character of copy string
  //    len: the number of characters to be copied
  // )

  // check ipv6 address service
  // exp: [2001:db8:85a3::8a2e:370:7334]:80
  //      |                            | |-service
  //      |                            |-endipv6
  //       |-- -- -- node -- -- -- -- |
  if (!host.empty() && host[0] == '[') {
    const char *endipv6 =
        (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
    if (endipv6) {
      // TODO: check out of range
      // check whether has port number
      if (*(endipv6 + 1) == ':') {
        service = endipv6 + 2;
      }
      node = host.substr(1, endipv6 - host.c_str() - 1);
    }
  }

  // check node service, ipv4(192.168.0.1:80), ipv6(not [::320]:8080)
  if (node.empty()) {
    service = (const char *)memchr(host.c_str(), ':', host.size());
    if (service) {
      // no more colons after this
      if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
        node = host.substr(0, service - host.c_str());
        ++service;
      }
    }
  }

  if (node.empty()) {
    node = host;
  }

  // replace `gethostbyname()` (only support ipv4)
  // - getaddrinfo(
  //    nodename: domain name, ipv4 address, ipv6 address
  //    servname: decimal port number
  //    hints: user specified struct addrinfo
  //    res: struct addrinfo pointer(similar as list), should using
  //    freeaddrino() to release this memory
  // )
  int error = getaddrinfo(node.c_str(), service, &hints, &results);
  if (error) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "Address::Lookup getaddrinfo(" << host << ", " << family << ", "
        << type << ", "
        << ") err=" << error << " errstr=" << gai_strerror(error);
    return false;
  }

  next = results;
  while (next) {
    // parsing into ipv4, ipv6, unix address
    result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    // APEXSTORM_LOG_DEBUG(g_logger)
    //     << ((sockaddr_in *)next->ai_addr)->sin_addr.s_addr;
    next = next->ai_next;
  }

  freeaddrinfo(results);
  return true;
}

bool Address::GetInterFaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
    int family) {
  // get interface information <ifaddrs.h>
  struct ifaddrs *next, *results;

  if (getifaddrs(&results) != 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "Address::GetInterFaceAddresses getifaddrs err=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }

  try {
    for (next = results; next; next = next->ifa_next) {
      Address::ptr addr;
      // in ipv4, prefix is 24 or 32, in ipv6 prefix is 64 or 128
      // prefix(netmask) length is unknown
      uint32_t prefix_len = ~0u;
      // filter the interface protocol cluster
      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
        continue;
      }
      switch (next->ifa_addr->sa_family) {
      case AF_INET: {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
        uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
        prefix_len = CountBytes(netmask);
      } break;
      case AF_INET6: {
        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
        in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
        prefix_len = 0;
        for (uint32_t i = 0; i < 16; ++i) {
          prefix_len += CountBytes(
              netmask.s6_addr[i]); /* uint8_t	__u6_addr8[16]; */
        }
      } break;
      default:
        break;
      }

      if (addr) {
        result.insert(
            std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
      }
    }
  } catch (...) {
    APEXSTORM_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return true;
}

bool Address::GetInterFaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t>> &result,
    const std::string &iface, int family) {
  if (iface.empty() || iface == "*") {
    if (family == AF_INET || family == AF_UNSPEC) {
      result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
    }
    if (family == AF_INET6 || family == AF_UNSPEC) {
      result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
    }
    return true;
  }

  std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
  if (!GetInterFaceAddresses(results, family)) {
    return false;
  }

  // Finds a subsequence matching given key.
  auto its = results.equal_range(iface);
  for (; its.first != its.second; ++its.first) {
    result.push_back(its.first->second);
  }
  return true;
}

int Address::getFamily() const { return getAddr()->sa_family; }

std::string Address::toString() const {
  std::stringstream ss;
  insert(ss);
  return ss.str();
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
  if (addr == nullptr) {
    return nullptr;
  }

  Address::ptr result;
  switch (addr->sa_family) {
  case AF_INET:
    result.reset(new IPv4Address(*(const sockaddr_in *)addr));
    break;
  case AF_INET6:
    result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
    break;
  default:
    result.reset(new UnknownAddress(*addr));
    break;
  }
  return result;
}

bool Address::operator<(const Address &rhs) const {
  socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
  int result = memcmp(getAddr(), rhs.getAddr(), minlen);
  if (result < 0) {
    return true;
  } else if (result > 0) {
    return false;
  } else if (getAddrLen() < rhs.getAddrLen()) {
    return true;
  }
  return false;
}

bool Address::operator==(const Address &rhs) const {
  return getAddrLen() == rhs.getAddrLen() &&
         memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
  addrinfo hints, *results;
  memset(&hints, 0, sizeof(hints));

  // hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;

  int error = getaddrinfo(address, NULL, &hints, &results);
  if (error) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "IPAddress::Create(" << address << ", " << port
        << ") error=" << errno << " errstr=" << strerror(errno);
    return nullptr;
  }

  try {
    IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
        Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));

    if (result) {
      result->setPort(port);
    }

    freeaddrinfo(results);
    return result;
  } catch (...) {
    freeaddrinfo(results);
    return nullptr;
  }
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
  IPv4Address::ptr rt(new IPv4Address);
  rt->m_addr.sin_port = byteswapOnLittleEndian(port);

  // inet_pton(presentation to numeric)
  // - inet_pton(
  //    family: AF_INET, AF_INET6
  //    strptr:  char*
  //    addrptr: uint32_t
  // ) succeed: 1, invalid expr 0, error -1(EAFNOSUPPORT)

  int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
  if (result < 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "IPv4Address::Create(" << address << ", " << port
        << ") rt=" << result << " error=" << errno
        << " errstr=" << strerror(errno);
    return nullptr;
  }

  if (result == 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "IPv4Address::Create(" << address << ", " << port
        << ") rt=" << result << " invalid expression: " << address;
    return nullptr;
  }

  return rt;
}

const sockaddr *IPv4Address::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv4Address::getAddr() { return (sockaddr *)&m_addr; }

socklen_t IPv4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv4Address::insert(std::ostream &os) const {
  // 32 bits address
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
  os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
  // limit 0 ~ 32
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr |=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
  // limit 0 ~ 32
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr &=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr =
      ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const { return m_addr.sin_port; }

void IPv4Address::setPort(uint16_t v) {
  m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::IPv6Address() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const char *address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian(port);
  memcpy(&m_addr.sin6_addr, address, 16);
}

IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
  IPv6Address::ptr rt(new IPv6Address);
  rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
  int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
  if (result < 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "IPv6Address::Create(" << address << ", " << port
        << ") rt=" << result << " error=" << errno
        << " errstr=" << strerror(errno);
    return nullptr;
  }

  if (result == 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "IPv6Address::Create(" << address << ", " << port
        << ") rt=" << result << " invalid expression: " << address;
    return nullptr;
  }

  return rt;
}

const sockaddr *IPv6Address::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv6Address::getAddr() { return (sockaddr *)&m_addr; }

socklen_t IPv6Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv6Address::insert(std::ostream &os) const {
  os << "[";
  uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
  bool used_zero = false;
  for (size_t i = 0; i < 8; ++i) {
    if (addr[i] == 0 && !used_zero) {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zero) {
      os << ":";
      used_zero = true;
    }
    if (i) {
      os << ":";
    }
    os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
  }

  if (!used_zero && addr[7] == 0) {
    os << "::";
  }

  os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
  return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  baddr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);

  for (size_t i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }

  return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  baddr.sin6_addr.s6_addr[prefix_len / 8] &=
      CreateMask<uint8_t>(prefix_len % 8);

  for (size_t i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0x00;
  }

  return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in6 subnet(m_addr);
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;
  subnet.sin6_addr.s6_addr[prefix_len / 8] =
      ~CreateMask<uint8_t>(prefix_len % 8);

  for (uint32_t i = 0; i < prefix_len / 8; ++i) {
    subnet.sin6_addr.s6_addr[i] = 0xFF;
  }

  return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
  m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string &path) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = path.size() + 1;

  if (!path.empty() && path[0] == '\0') {
    --m_length;
  }
  if (m_length > sizeof(m_addr.sun_path)) {
    throw std::logic_error("path too long");
  }
  memcpy(m_addr.sun_path, path.c_str(), m_length);
  m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr *UnixAddress::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *UnixAddress::getAddr() { return (sockaddr *)&m_addr; }

socklen_t UnixAddress::getAddrLen() const { return m_length; }

void UnixAddress::setAddrLen(uint32_t len) { m_length = len; }

std::ostream &UnixAddress::insert(std::ostream &os) const {
  if (m_length > offsetof(sockaddr_un, sun_path) &&
      m_addr.sun_path[0] == '\0') {
    return os << "\\0"
              << std::string(m_addr.sun_path + 1,
                             m_length - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sa_family = family;
}

const sockaddr *UnknownAddress::getAddr() const { return &m_addr; }

sockaddr *UnknownAddress::getAddr() { return &m_addr; }

socklen_t UnknownAddress::getAddrLen() const { return sizeof(m_addr); }

std::ostream &UnknownAddress::insert(std::ostream &os) const {
  os << "[UnknownAddress family=" << m_addr.sa_family << "]";
  return os;
}

} // namespace apexstorm