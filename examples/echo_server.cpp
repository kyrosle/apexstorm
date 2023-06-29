#include "address.h"
#include "apexstorm.h"
#include "bytearray.h"
#include "iomanager.h"
#include "log.h"
#include "socket.h"
#include "tcp_server.h"
#include <bits/types/struct_iovec.h>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

// Implementation of TcpServer
class EchoServer : public apexstorm::TcpServer {
public:
  // server handle type:
  // 1: for text transport
  // 2: for binary transport
  EchoServer(int type);

  // override the handling new socket client method
  void handleClient(apexstorm::Socket::ptr client) override;

private:
  // server type
  int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(apexstorm::Socket::ptr client) {
  APEXSTORM_LOG_INFO(g_logger) << "handleClient " << *client;

  // structure for byte array
  apexstorm::ByteArray::ptr ba(new apexstorm::ByteArray);

  while (true) {
    ba->clear();
    std::vector<iovec> iovs;

    ba->getWriteBuffers(iovs, 1024);

    int rt = client->recv(&iovs[0], iovs.size());

    if (rt == 0) {
      APEXSTORM_LOG_INFO(g_logger) << "client close: " << *client;
      break;
    } else if (rt < 0) {
      APEXSTORM_LOG_INFO(g_logger)
          << "client error rt=" << rt << " errno=" << errno
          << " errstr=" << strerror(errno);
    }

    // ba->setPosition(ba->getPosition() + rt);
    // reset the byte array pointer
    ba->setPosition(0);

    APEXSTORM_LOG_INFO(g_logger)
        << "recv rt=" << rt
        << " data=" << std::string((char *)iovs[0].iov_base, rt);

    if (m_type == 1) { // text
      std::cout << ba->toString();
    } else { // binary hex output
      std::cout << ba->toHexString();
    }

    // flush the line cache
    std::cout.flush();
  }
}

int type = 1;

// execution task
void run() {
  EchoServer::ptr es(new EchoServer(type));
  auto addr = apexstorm::Address::LookupAny("0.0.0.0:8020");
  while (!es->bind(addr)) {
    sleep(2);
  }
  es->start();
}

// xmake r echo_server -t/-b
int main(int argc, char **argv) {
  if (argc < 2) {
    APEXSTORM_LOG_INFO(g_logger)
        << "used as " << std::endl
        << "[" << argv[0] << " -t] or\n[" << argv[0] << " -b]";
    return 0;
  }
  if (!strcmp(argv[1], "-b")) {
    type = 2;
  }
  apexstorm::IOManager iom(2);
  iom.schedule(run);
  return 0;
}