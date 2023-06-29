#include "address.h"
#include "iomanager.h"
#include "log.h"
#include "tcp_server.h"
#include <hook.h>
#include <vector>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void run() {
  auto addr = apexstorm::Address::LookupAny("0.0.0.0:80");
  // auto addr2 =
  //     apexstorm::UnixAddress::ptr(new
  //     apexstorm::UnixAddress("/tmp/unix_addr"));

  std::vector<apexstorm::Address::ptr> addrs;
  addrs.push_back(addr);
  // addrs.push_back(addr2);

  apexstorm::TcpServer::ptr tcp_server(new apexstorm::TcpServer);
  std::vector<apexstorm::Address::ptr> fails;
  while (!tcp_server->bind(addrs, fails)) {
    sleep(2);
    addrs.swap(fails);
    fails.clear();
  }
  tcp_server->start();
}
int main() {
  apexstorm::IOManager iom(2);
  iom.schedule(run);
  return 0;
}