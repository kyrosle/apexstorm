#ifndef __APEXSTORM_HTTP_SESSION_H__
#define __APEXSTORM_HTTP_SESSION_H__

#include "http/http.h"
#include "socket.h"
#include "socket_stream.h"
#include <memory>

namespace apexstorm {

namespace http {

class HttpSession : public SocketStream {
public:
  typedef std::shared_ptr<HttpSession> ptr;

  HttpSession(Socket::ptr sock, bool owner = true);

  HttpRequest::ptr recvRequest();
  int sendResponse(HttpResponse::ptr rsp);
};

} // namespace http
} // namespace apexstorm

#endif
