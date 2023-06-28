/**
 * @file http_parser.h
 * @author your name (you@domain.com)
 * @brief HTTP protocol parser.
 * @version 0.1
 * @date 2023-06-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_HTTP_PARSER_H
#define __APEXSTORM_HTTP_PARSER_H

#include <memory>

#include "http.h"
#include "http11_common.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace apexstorm {
namespace http {

// HTTP Request Parser
class HttpRequestParer {
public:
  typedef std::shared_ptr<HttpRequestParer> ptr;

  // Constructor
  HttpRequestParer();

  // Parse the protocol
  // @param: data - the protocol content
  // @param: len  - the length of the content
  size_t execute(char *data, size_t len);

  // Return whether parsing succeeded.
  int isFinished();

  // Return whether parsing meets error.
  int hasError();

  // Set teh error code.
  void setError(int v) { m_error = v; }

  // Get the `HttpRequest` structure.
  HttpRequest::ptr getData() const { return m_data; }

  // Get the Message total length.
  uint64_t getContentLength();

private:
  // http parser
  http_parser m_parser;
  // HttpRequest structure
  HttpRequest::ptr m_data;
  // Error code:
  // 1000: invalid method
  // 1001: invalid version
  // 1002: invalid field
  int m_error;
};

// HTTP Response Parser
class HttpResponseParer {
public:
  typedef std::shared_ptr<HttpResponseParer> ptr;

  // Constructor
  HttpResponseParer();

  // Parse the protocol
  // @param: data - the protocol content
  // @param: len  - the length of the content
  size_t execute(char *data, size_t len);

  // Return whether parsing succeeded.
  int isFinished();

  // Return whether parsing meets error.
  int hasError();

  // Set teh error code.
  void setError(int v) { m_error = v; }

  // Get the `HttpResponse` structure.
  HttpResponse::ptr getData() const { return m_data; }

  // Get the Message total length.
  uint64_t getContentLength();

private:
  // httpclient parser
  httpclient_parser m_parser;
  // HttpResponse structure
  HttpResponse::ptr m_data;
  // Error code:
  // 1001: invalid version
  // 1002: invalid field
  int m_error;
};

} // namespace http
} // namespace apexstorm
#endif