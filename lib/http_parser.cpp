
#include "http/http_parser.h"
#include "http/http.h"
#include "http11_common.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace apexstorm {
namespace http {

void on_request_method(void *data, const char *at, size_t len) {}
void on_request_uri(void *data, const char *at, size_t len) {}
void on_request_fragment(void *data, const char *at, size_t len) {}
void on_request_path(void *data, const char *at, size_t len) {}
void on_request_query(void *data, const char *at, size_t len) {}
void on_request_version(void *data, const char *at, size_t len) {}
void on_request_header_done(void *data, const char *at, size_t len) {}
void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {}

HttpRequestParer::HttpRequestParer() {
  m_data.reset(new apexstorm::http::HttpRequest);
  http_parser_init(&m_parser);
  m_parser.request_method = on_request_method;
  m_parser.request_uri = on_request_method;
  m_parser.fragment = on_request_fragment;
  m_parser.request_path = on_request_path;
  m_parser.query_string = on_request_query;
  m_parser.http_version = on_request_version;
  m_parser.header_done = on_request_header_done;
  m_parser.http_field = on_request_http_field;
}

size_t HttpRequestParer::execute(const char *data, size_t len, size_t offset) {
  return 0;
}
int HttpRequestParer::isFinished() const { return 0; }
int HttpRequestParer::hasError() const { return 0; }

HttpResponseParer::HttpResponseParer() {}

size_t HttpResponseParer::execute(const char *data, size_t len, size_t offset) {
  return 0;
}

int HttpResponseParer::isFinished() const { return 0; }
int HttpResponseParer::hasError() const { return 0; }

} // namespace http
} // namespace apexstorm