#ifndef __APEXSTORM_HTTP_SERVER_H__
#define __APEXSTORM_HTTP_SERVER_H__

#include "http_session.h"
#include "iomanager.h"
#include "tcp_server.h"
#include <memory>

namespace apexstorm {
namespace http {

class HttpServer : public TcpServer {
public:
  typedef std::shared_ptr<HttpServer> ptr;

  HttpServer(
      bool keepalive = false,
      apexstorm::IOManager *worker = apexstorm::IOManager::IOManager::GetThis(),
      apexstorm::IOManager *accept_worker =
          apexstorm::IOManager::IOManager::GetThis());

protected:
  virtual void handleClient(Socket::ptr client) override;

private:
  bool m_isKeepalive;
};

} // namespace http
} // namespace apexstorm

#endif