#include "http_server.h"
#include "http.h"
#include "http_session.h"
#include "iomanager.h"
#include "log.h"
#include "tcp_server.h"
#include <cstring>
namespace apexstorm {
namespace http {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, apexstorm::IOManager *worker,
                       apexstorm::IOManager *accept_worker)
    : TcpServer(worker, accept_worker), m_isKeepalive(keepalive) {}

void HttpServer::handleClient(Socket::ptr client) {
  HttpSession::ptr session(new HttpSession(client));
  do {
    auto req = session->recvRequest();
    if (!req) {
      APEXSTORM_LOG_WARN(g_logger)
          << "recv http request fail, errno=" << errno
          << " errstr=" << strerror(errno) << " client:" << *client;
      break;
    }

    HttpResponse::ptr rsp(
        new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));

    rsp->setBody("hello apexstorm");

    APEXSTORM_LOG_INFO(g_logger) << "request: " << std::endl << *req;

    APEXSTORM_LOG_INFO(g_logger) << "response: " << std::endl << *rsp;

    session->sendResponse(rsp);

  } while (m_isKeepalive);
  session->close();
}

} // namespace http
} // namespace apexstorm