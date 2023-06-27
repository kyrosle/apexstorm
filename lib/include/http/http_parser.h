#ifndef __APEXSTORM_HTTP_PARSER_H
#define __APEXSTORM_HTTP_PARSER_H

#include <memory>

#include "http.h"
#include "http11_common.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace apexstorm {
namespace http {

class HttpRequestParer {
public:
  typedef std::shared_ptr<HttpRequestParer> ptr;

  HttpRequestParer();

  size_t execute(const char *data, size_t len, size_t offset);
  int isFinished() const;
  int hasError() const;

private:
  http_parser m_parser;
  HttpRequest::ptr m_data;
  int m_error;
};

class HttpResponseParer {
public:
  typedef std::shared_ptr<HttpResponseParer> ptr;

  HttpResponseParer();

  size_t execute(const char *data, size_t len, size_t offset);
  int isFinished() const;
  int hasError() const;

private:
  httpclient_parser m_parser;
  HttpResponse::ptr m_data;
  int m_error;
};
} // namespace http
} // namespace apexstorm
#endif