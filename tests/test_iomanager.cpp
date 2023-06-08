#include "../include/apexstorm.h"
#include "../include/iomanager.h"
#include "log.h"
#include "macro.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

int sock = 0;

void test_fiber() {
  APEXSTORM_LOG_INFO(g_logger) << "test fiber sock=" << sock;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  int flag = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flag | O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

  if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
  } else if (errno == EINPROGRESS) {
    APEXSTORM_LOG_INFO(g_logger)
        << "add event errno=" << errno << " " << strerror(errno);

    apexstorm::IOManager::GetThis()->addEvent(
        sock, apexstorm::IOManager::Event::READ,
        []() { APEXSTORM_LOG_INFO(g_logger) << "connected"; });

  } else {
    APEXSTORM_LOG_INFO(g_logger)
        << "else errno=" << errno << " " << strerror(errno);
  }
}

void test1() {
  // the uint32 value of `EPOLLIN` and `EPOLLOUT`
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  apexstorm::IOManager iom(2, false, "thread");
  iom.schedule(&test_fiber);
}

int main() {
  test1();
  return 0;
}