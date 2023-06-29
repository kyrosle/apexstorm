#include "tcp_server.h"
#include "address.h"
#include "config.h"
#include "log.h"
#include "socket.h"
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

namespace apexstorm {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

// tcp server read receive timeout
static apexstorm::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    apexstorm::Config::Lookup("tcp_server.read_timeout",
                              (uint64_t)(60 * 2000 * 2),
                              "tcp_server read_timeout");

TcpServer::TcpServer(apexstorm::IOManager *worker,
                     apexstorm::IOManager *accept_worker)
    : m_worker(worker), m_recvTimeout(g_tcp_server_read_timeout->getValue()),
      m_accept_worker(accept_worker), m_name("apexstorm/1.0.0"),
      m_isStop(true) {}

TcpServer::~TcpServer() {
  for (auto &i : m_socks) {
    i->close();
  }
  m_socks.clear();
}

bool TcpServer::bind(apexstorm::Address::ptr addr) {
  std::vector<Address::ptr> addrs;
  std::vector<Address::ptr> fails;
  addrs.push_back(addr);
  return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<apexstorm::Address::ptr> &addrs,
                     std::vector<apexstorm::Address::ptr> &fails) {
  for (auto &addr : addrs) {
    // create tcp socket
    Socket::ptr sock = Socket::CreateTCP(addr);

    // bind address
    if (!sock->bind(addr)) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "bind fail errno=" << errno << " errstr=" << strerror(errno)
          << " addr=[" << addr->toString() << "]";
      fails.push_back(addr);
      continue;
    }

    // socket trans into listening
    if (!sock->listen()) {
      APEXSTORM_LOG_ERROR(g_logger)
          << "listen fail errno=" << errno << " errstr=" << strerror(errno)
          << " addr=[" << addr->toString() << "]";
      fails.push_back(addr);
      continue;
    }

    // store the reference of successfully socket
    m_socks.push_back(sock);
  }

  if (!fails.empty()) {
    m_socks.clear();
    return false;
  }

  for (auto &i : m_socks) {
    APEXSTORM_LOG_INFO(g_logger) << "server bind success: " << *i;
  }

  return true;
}

void TcpServer::startAccept(Socket::ptr sock) {
  while (!m_isStop) {
    // socket accepting
    Socket::ptr client = sock->accept();

    if (client) {
      client->setRecvTimeout(m_recvTimeout);
      m_worker->schedule(
          std::bind(&TcpServer::handleClient, shared_from_this(), client));
    } else {
      APEXSTORM_LOG_ERROR(g_logger)
          << "accept errno=" << errno << " errstr=" << strerror(errno);
    }
  }
}

bool TcpServer::start() {
  if (!m_isStop) {
    return true;
  }
  m_isStop = false;
  for (auto &sock : m_socks) {
    // add sock accept() into accept_worker scheduler
    m_accept_worker->schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), sock));
  }
  return true;
}

void TcpServer::stop() {
  m_isStop = true;
  auto self = shared_from_this();

  // add cancel task into accept_worker scheduler
  m_accept_worker->schedule([self]() {
    for (auto &sock : self->m_socks) {
      sock->cancelAll();
      sock->close();
    }
  });
}

void TcpServer::handleClient(Socket::ptr client) {
  APEXSTORM_LOG_INFO(g_logger) << "handleClient: " << *client;
}

} // namespace apexstorm