#include "address.h"
#include "http_server.h"
#include "iomanager.h"
#include "log.h"

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void run() {
  apexstorm::http::HttpServer::ptr server(new apexstorm::http::HttpServer);
  apexstorm::Address::ptr addr =
      apexstorm::Address::LookupAnyIPAddress("0.0.0.0:8020");
  while (!server->bind(addr)) {
    sleep(2);
  }
  server->start();
}

int main() {
  apexstorm::IOManager iom(2);
  iom.schedule(run);
  return 0;
}