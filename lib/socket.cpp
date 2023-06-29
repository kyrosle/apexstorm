#include "socket.h"
#include "address.h"
#include "fdmanager.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include <asm-generic/socket.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(apexstorm::Address::ptr address) {
  Socket::ptr sock(new Socket(address->getFamily(), Type::TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDP(apexstorm::Address::ptr address) {
  Socket::ptr sock(new Socket(address->getFamily(), Type::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
  Socket::ptr sock(new Socket(Family::IPv4, Type::TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
  Socket::ptr sock(new Socket(Family::IPv4, Type::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
  Socket::ptr sock(new Socket(Family::IPv6, Type::TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
  Socket::ptr sock(new Socket(Family::IPv6, Type::UDP, 0));
  return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
  Socket::ptr sock(new Socket(Family::Unix, Type::TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
  Socket::ptr sock(new Socket(Family::Unix, Type::UDP, 0));
  return sock;
}

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1), m_family(family), m_type(type), m_protocol(protocol),
      m_isConnected(false) {}

Socket::~Socket() { close(); }

int64_t Socket::getSendTimeout() {
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_SNDTIMEO);
  }
  return -1;
}

void Socket::setSendTimeout(int64_t timeout) {
  struct timeval tv {
    int(timeout / 1000) /* second */,
        int(timeout % 1000 * 1000) /* millisecond */
  };
  setOption(SOL_SOCKET /* setsockopt - SOL_SOCKET */,
            SO_SNDTIMEO /* socket send timeout */, tv);
}

int64_t Socket::getRecvTimeout() {
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_RCVTIMEO);
  }
  return -1;
}

void Socket::setRecvTimeout(int64_t timeout) {
  struct timeval tv {
    int(timeout / 1000) /* second */,
        int(timeout % 1000 * 1000) /* millisecond */
  };
  setOption(SOL_SOCKET /* setsockopt - SOL_SOCKET */,
            SO_RCVTIMEO /* socket receive timeout */, tv);
}

bool Socket::getOption(int level, int option, void *result, size_t *len) {
  // hooked
  int rt = getsockopt(m_sock, level, option, result, (socklen_t *)len);
  if (rt) {
    APEXSTORM_LOG_DEBUG(g_logger)
        << "getOption sock=" << m_sock << " level=" << level
        << " option=" << option << " errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  } else {
    return true;
  }
}

bool Socket::setOption(int level, int option, const void *value, size_t len) {
  // hooked
  if (setsockopt(m_sock, level, option, value, (socklen_t)len)) {
    APEXSTORM_LOG_DEBUG(g_logger)
        << "setOption sock=" << m_sock << " level=" << level
        << " option=" << option << " errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  } else {
    return true;
  }
}

Socket::ptr Socket::accept() {
  Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
  // hooked
  int newsock = ::accept(m_sock, nullptr, nullptr);
  if (newsock == -1) {
    APEXSTORM_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno
                                  << " errstr=" << strerror(errno);
    return nullptr;
  }
  if (sock->init(newsock)) {
    return sock;
  }
  return nullptr;
}

bool Socket::init(int sock) {
  // initialize the sock
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
  if (ctx && ctx->isSocket() && !ctx->isClosed()) {
    m_sock = sock;
    m_isConnected = true;
    initSock();
    getLocalAddress();
    getRemoteAddress();
    return true;
  }
  return false;
}

// called by `Socket::init` and `Socket::newSock()`
void Socket::initSock() {
  int val = 1;
  setOption(SOL_SOCKET /* setsockopt - SOL_SOCKET */,
            SO_REUSEADDR /* reuse address */, val /* 1 */);
  if (m_type ==
      SOCK_STREAM /* Sequenced, reliable, connection-based byte streams. */) {
    setOption(IPPROTO_TCP /* Transmission Control Protocol. */,
              TCP_NODELAY /* Don't delay send to coalesce packets */,
              val /* 1 */);
  }
}

// new socket in `bind` and `connect`
void Socket::newSock() {
  m_sock = socket(m_family, m_type, m_protocol);
  if (APEXSTORM_LIKELY(m_sock != -1)) {
    initSock();
  } else {
    APEXSTORM_LOG_ERROR(g_logger)
        << "socket(" << m_family << ", " << m_type << ", " << m_protocol
        << ") errno=" << errno << " errstr=" << strerror(errno);
  }
}

bool Socket::bind(const Address::ptr addr) {
  if (!isValid()) {
    newSock();
    if (APEXSTORM_UNLIKELY(!isValid())) {
      return false;
    }
  }

  if (APEXSTORM_UNLIKELY(addr->getFamily() != m_family)) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "bind sock.family(" << m_family << ") addr.family("
        << addr->getFamily() << ") not equal, addr=" << addr->toString();
    return false;
  }

  // hooked
  if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "bind error sock=" << m_sock << " errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }
  // initialize local address
  getLocalAddress();
  return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
  if (!isValid()) {
    newSock();
    if (APEXSTORM_UNLIKELY(!isValid())) {
      return false;
    }
  }

  if (APEXSTORM_UNLIKELY(addr->getFamily() != m_family)) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "connect sock.family(" << m_family << ") addr.family("
        << addr->getFamily() << ") not equal, addr=" << addr->toString();
    return false;
  }

  if (timeout_ms == (uint64_t)-1) {
    // hooked
    if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "sock=" << m_sock << " connect(" << addr->toString()
          << ") error errno=" << errno << " errstr=" << strerror(errno);
      close();
      return false;
    }
  } else {
    // hooked
    if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(),
                               timeout_ms)) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "sock=" << m_sock << " connect(" << addr->toString()
          << ") timeout=" << timeout_ms << " error errno=" << errno
          << " errstr=" << strerror(errno);
      close();
      return false;
    }
  }

  m_isConnected = true;
  // initialize remote address and local address
  getRemoteAddress();
  getLocalAddress();
  return true;
}

bool Socket::listen(int backlog) {
  if (!isValid()) {
    APEXSTORM_LOG_ERROR(g_logger) << "listen error sock=-1";
    return false;
  }
  // hooked
  if (::listen(m_sock, backlog) != 0) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "listen error errno=" << errno << " errstr=" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::close() {
  if (!m_isConnected && m_sock == -1) {
    // already closed
    return true;
  }

  m_isConnected = false;
  if (m_sock != -1) {
    // hooked
    ::close(m_sock);
    m_sock = -1;
  }
  return false;
}

int Socket::send(const void *buffer, size_t length, int flags) {
  if (isConnected()) {
    // hooked
    return ::send(m_sock, buffer, length, flags);
  }
  return -1;
}

int Socket::send(const iovec *buffers, size_t length, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffers; /* Vector of data to send/receive into. */
    msg.msg_iovlen = length;        /* Length of address data. */
    // hooked
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    // hooked
    return ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                    to->getAddrLen());
  }
  return -1;
}

int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffers;
    msg.msg_iovlen = length;
    // optional address
    msg.msg_name = to->getAddr(); /* Address to send to/receive from. */
    // sizeof address
    msg.msg_namelen = to->getAddrLen(); /* Length of address data. */
    // hooked
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) {
  if (isConnected()) {
    // hooked
    return ::recv(m_sock, buffer, length, flags);
  }
  return -1;
}

int Socket::recv(iovec *buffers, size_t length, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffers;
    msg.msg_iovlen = length;
    // hooked
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recvFrom(void *buffer, size_t length, const Address::ptr from,
                     int flags) {
  if (isConnected()) {
    // const unsigned int -> socklen_t
    socklen_t len = from->getAddrLen();
    // hooked
    return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
  }
  return -1;
}

int Socket::recvFrom(iovec *buffers, size_t length, const Address::ptr from,
                     int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffers;
    msg.msg_iovlen = length;
    // optional address
    msg.msg_name = from->getAddr();
    // sizeof address
    msg.msg_namelen = from->getAddrLen();
    // hooked
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

Address::ptr Socket::getRemoteAddress() {
  if (m_remoteAddress) {
    return m_remoteAddress;
  }

  // initialize remote address
  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknownAddress(m_family));
    break;
  }
  socklen_t addrlen = result->getAddrLen();
  if (getpeername(m_sock, result->getAddr(), &addrlen)) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "getpeername error sock=" << m_sock << " errno=" << errno
        << " errstr=" << strerror(errno);
    return Address::ptr(new UnknownAddress(m_family));
  }
  // resize the address length if it's unix socket
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  m_remoteAddress = result;
  return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
  if (m_localAddress) {
    return m_localAddress;
  }

  // initialize local address
  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknownAddress(m_family));
    break;
  }
  socklen_t addrlen = result->getAddrLen();

  // - getsockname()
  // returns  the  current address to which the socket sockfd is bound.
  // storing in `result->getAddr()` (sockaddr*)
  if (getsockname(m_sock, result->getAddr(), &addrlen)) {
    APEXSTORM_LOG_ERROR(g_logger)
        << "getsockname error sock=" << m_sock << " errno=" << errno
        << " errstr=" << strerror(errno);
    return Address::ptr(new UnknownAddress(m_family));
  }
  // resize the address length if it's unix socket
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  m_localAddress = result;
  return m_localAddress;
}

bool Socket::isValid() const { return m_sock != -1; }

int Socket::getError() {
  int error = 0;
  size_t len = sizeof(error);
  if (!getOption(SOL_SOCKET /* getsockopt - SOL_SOCKET */,
                 SO_ERROR /* get the error number */, &error, &len)) {
    // if the getsockopt() -> -1 failed, fetch the `errno` value to error
    error = errno;
  }
  return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
  os << "[Socket sock=" << m_sock << " is_connected=" << m_isConnected
     << " family=" << m_family << " type=" << m_type
     << " protocol=" << m_protocol;
  if (m_localAddress) {
    os << " local_address=" << m_localAddress->toString();
  }
  if (m_remoteAddress) {
    os << " remote_address=" << m_remoteAddress->toString();
  }
  os << "]";
  return os;
}

bool Socket::cancelRead() {
  return IOManager::GetThis()->cancelEvent(m_sock,
                                           apexstorm::IOManager::Event::READ);
}

bool Socket::cancelWrite() {
  return IOManager::GetThis()->cancelEvent(m_sock,
                                           apexstorm::IOManager::Event::WRITE);
}

bool Socket::cancelAccept() {
  return IOManager::GetThis()->cancelEvent(m_sock,
                                           apexstorm::IOManager::Event::READ);
}

bool Socket::cancelAll() { return IOManager::GetThis()->cancelAll(m_sock); }

} // namespace apexstorm