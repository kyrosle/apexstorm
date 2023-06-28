#include "config.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "http/http.h"
#include "http/http_parser.h"
#include "http11_common.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include "log.h"

namespace apexstorm {
namespace http {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

static apexstorm::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    apexstorm::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024),
                              "http request buffer size");
static apexstorm::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    apexstorm::Config::Lookup("http.request.max_body_size",
                              (uint64_t)(64 * 1024 * 1024),
                              "http request max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;

struct _RequestSizeIniter {
  _RequestSizeIniter() {
    s_http_request_buffer_size = g_http_request_buffer_size->getValue();
    s_http_request_max_body_size = g_http_request_max_body_size->getValue();

    g_http_request_buffer_size->addListener(
        [](const uint64_t &ov, const uint64_t &nv) {
          s_http_request_buffer_size = nv;
        });

    g_http_request_max_body_size->addListener(
        [](const uint64_t &ov, const uint64_t &nv) {
          s_http_request_max_body_size = nv;
        });
  }
};
static _RequestSizeIniter _init;

void on_request_method(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  HttpMethod m = CharsToHttpMethod(at);

  if (m == HttpMethod::HIT_INVALID_METHOD) {
    APEXSTORM_LOG_WARN(g_logger) << "invalid http request method ";
    parser->setError(1000);
    return;
  }
  parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t len) {}

void on_request_fragment(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  parser->getData()->setFragment(std::string(at, len));
}

void on_request_path(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  parser->getData()->setPath(std::string(at, len));
}

void on_request_query(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  parser->getData()->setQuery(std::string(at, len));
}

void on_request_version(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", len) == 0) {
    v = 0x10;
  } else {
    APEXSTORM_LOG_WARN(g_logger)
        << "invalid http request version: " << std::string(at, len);
    parser->setError(1001);
    return;
  }
  parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t len) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
}

void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {
  HttpRequestParer *parser = static_cast<HttpRequestParer *>(data);
  if (flen == 0) {
    APEXSTORM_LOG_WARN(g_logger) << "invalid http request field length == 0";
    parser->setError(1002);
  }
  parser->getData()->setHeader(std::string(field, flen),
                               std::string(value, vlen));
}

HttpRequestParer::HttpRequestParer() {
  m_data.reset(new apexstorm::http::HttpRequest);
  http_parser_init(&m_parser);
  m_parser.request_method = on_request_method;
  m_parser.request_uri = on_request_uri;
  m_parser.fragment = on_request_fragment;
  m_parser.request_path = on_request_path;
  m_parser.query_string = on_request_query;
  m_parser.http_version = on_request_version;
  m_parser.header_done = on_request_header_done;
  m_parser.http_field = on_request_http_field;
  m_parser.data = this;
}

// 1 : success
// -1: has error
// >0: the bytes has solved, and `data` useable data length is len - v
size_t HttpRequestParer::execute(char *data, size_t len) {
  size_t offset = http_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + offset, (len - offset));
  return offset;
}

int HttpRequestParer::isFinished() { return http_parser_finish(&m_parser); }

int HttpRequestParer::hasError() {
  return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParer::getContentLength() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

void on_response_reason(void *data, const char *at, size_t length) {
  HttpResponseParer *parser = static_cast<HttpResponseParer *>(data);
  parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
  HttpResponseParer *parser = static_cast<HttpResponseParer *>(data);
  HttpStatus status = (HttpStatus)(atoi(at));
  parser->getData()->setStatus(status);
}

void on_response_version(void *data, const char *at, size_t length) {
  HttpResponseParer *parser = static_cast<HttpResponseParer *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    APEXSTORM_LOG_WARN(g_logger)
        << "invalid http response version: " << std::string(at, length);
    parser->setError(1001);
    return;
  }
  parser->getData()->setVersion(v);
}

void on_response_chunk(void *data, const char *at, size_t length) {}
void on_response_header_done(void *data, const char *at, size_t length) {}
void on_response_last_chunk(void *data, const char *at, size_t length) {}

void on_response_header_field(void *data, const char *field, size_t flen,
                              const char *value, size_t vlen) {
  HttpResponseParer *parser = static_cast<HttpResponseParer *>(data);
  if (flen == 0) {
    APEXSTORM_LOG_WARN(g_logger) << "invalid http response field length == 0";
    parser->setError(1002);
  }
  parser->getData()->setHeader(std::string(field, flen),
                               std::string(value, vlen));
}

HttpResponseParer::HttpResponseParer() : m_error(0) {
  m_data.reset(new apexstorm::http::HttpResponse);
  httpclient_parser_init(&m_parser);
  m_parser.reason_phrase = on_response_reason;
  m_parser.status_code = on_response_status;
  m_parser.chunk_size = on_response_chunk;
  m_parser.http_version = on_response_version;
  m_parser.header_done = on_response_header_done;
  m_parser.last_chunk = on_response_last_chunk;
  m_parser.http_field = on_response_header_field;
  m_parser.data = this;
}

size_t HttpResponseParer::execute(char *data, size_t len) {
  size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + offset, (len - offset));
  return offset;
}

int HttpResponseParer::isFinished() {
  return httpclient_parser_is_finished(&m_parser);
}
int HttpResponseParer::hasError() {
  return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParer::getContentLength() {
  return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

} // namespace http
} // namespace apexstorm