#include "address.h"
#include "iomanager.h"
#include "log.h"
#include "socket.h"
#include <string>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_socket() {
  apexstorm::IPAddress::ptr addr =
      apexstorm::Address::LookupAnyIPAddress("www.baidu.com");

  if (addr) {
    APEXSTORM_LOG_INFO(g_logger) << "get address: " << addr->toString();
  } else {
    APEXSTORM_LOG_ERROR(g_logger) << "get address fail";
  }

  apexstorm::Socket::ptr sock = apexstorm::Socket::CreateTCP(addr);
  addr->setPort(80);

  APEXSTORM_LOG_INFO(g_logger) << "addr=" << addr->toString();

  APEXSTORM_LOG_INFO(g_logger) << "sock=" << sock->getSocket();
  if (!sock->connect(addr)) {
    APEXSTORM_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
    return;
  } else {
    APEXSTORM_LOG_INFO(g_logger)
        << "connect " << addr->toString() << " connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\r\n";
  int rt = sock->send(buff, sizeof(buff));
  if (rt <= 0) {
    APEXSTORM_LOG_ERROR(g_logger) << "send failed rt=" << rt;
    return;
  }

  std::string buffs;
  buffs.resize(4096);
  rt = sock->recv(&buffs[0], buffs.size());

  if (rt <= 0) {
    APEXSTORM_LOG_ERROR(g_logger) << "recv failed rt=" << rt;
    return;
  }

  buffs.reserve(rt);
  APEXSTORM_LOG_INFO(g_logger) << buffs;
}

int main() {
  apexstorm::IOManager iom;
  iom.schedule(&test_socket);
  return 0;
}