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

  size_t execute(char *data, size_t len);
  int isFinished();
  int hasError();
  void setError(int v) { m_error = v; }

  HttpRequest::ptr getData() const { return m_data; }

  uint64_t getContentLength();

private:
  http_parser m_parser;
  HttpRequest::ptr m_data;
  // 1000: invalid method
  // 1001: invalid version
  // 1002: invalid field
  int m_error;
};

class HttpResponseParer {
public:
  typedef std::shared_ptr<HttpResponseParer> ptr;

  HttpResponseParer();

  size_t execute(char *data, size_t len);
  int isFinished();
  int hasError();
  void setError(int v) { m_error = v; }

  HttpResponse::ptr getData() const { return m_data; }

  uint64_t getContentLength();

private:
  httpclient_parser m_parser;
  HttpResponse::ptr m_data;
  // 1001: invalid version
  // 1002: invalid field
  int m_error;
};

} // namespace http
} // namespace apexstorm
#endif