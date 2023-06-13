#include "../include/hook.h"
#include "../include/iomanager.h"
#include "../include/log.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test_sleep() {
  apexstorm::IOManager iom(1);
  iom.schedule([]() {
    sleep(2);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 2";
  });

  iom.schedule([]() {
    sleep(3);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 3";
  });
  APEXSTORM_LOG_INFO(g_logger) << "test_sleep";
}

void test_socket() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  // addr.sin_port = htons(8080);
  // inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "110.242.68.66", &addr.sin_addr.s_addr);

  APEXSTORM_LOG_INFO(g_logger) << "begin connect";
  int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));
  APEXSTORM_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

  if (rt) {
    return;
  }

  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  rt = send(sock, data, sizeof(data), 0);
  APEXSTORM_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
  if (rt <= 0) {
    return;
  }

  std::string buff;
  buff.resize(4096);

  rt = recv(sock, &buff[0], buff.size(), 0);
  APEXSTORM_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

  if (rt <= 0) {
    return;
  }

  buff.resize(rt);
  APEXSTORM_LOG_INFO(g_logger) << buff;
}

int main() {
  // test_sleep();
  // test_socket();
  apexstorm::IOManager iom;
  iom.schedule(test_socket);
}