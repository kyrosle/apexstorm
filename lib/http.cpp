#include "http/http.h"
#include "log.h"
#include <cstdint>
#include <cstring>
#include <ostream>
#include <sstream>
#include <strings.h>

namespace apexstorm {

namespace http {

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_NAME("system");

HttpMethod StringToHttpMethod(const std::string &m) {
#define XX(num, name, string)                                                  \
  if (strcmp(#string, m.c_str()) == 0) {                                       \
    return HttpMethod::name;                                                   \
  }
  HTTP_METHOD_MAP(XX)
#undef XX
  return HttpMethod::HIT_INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char *m) {
#define XX(num, name, string)                                                  \
  if (strncmp(#string, m, strlen(#string)) == 0) {                             \
    return HttpMethod::name;                                                   \
  }
  HTTP_METHOD_MAP(XX)
#undef XX
  return HttpMethod::HIT_INVALID_METHOD;
}

static const char *s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char *HttpMethodToString(const HttpMethod &m) {
  uint32_t idx = (int)m;
  if (idx >= sizeof(s_method_string) / sizeof(s_method_string[0])) {
    return "<unknown>";
  }
  return s_method_string[idx];
}
const char *HttpStatusToString(const HttpStatus &s) {
  switch (s) {
#define XX(num, name, msg)                                                     \
  case HttpStatus::name:                                                       \
    return #msg;
    HTTP_STATUS_MAP(XX);
#undef XX
  default:
    return "<unknown>";
  }
}

bool CaseInsensitiveLess::operator()(const std::string &lhs,
                                     const std::string &rhs) const {
  return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET), m_version(version), m_close(close),
      m_path("/") {}

#define XX(var)                                                                \
  auto it = var.find(key);                                                     \
  return it == var.end() ? def : it->second;
std::string HttpRequest::getHeader(const std::string &key,
                                   const std::string &def) {
  XX(m_headers);
}
std::string HttpRequest::getParam(const std::string &key,
                                  const std::string &def) {
  XX(m_params);
}
std::string HttpRequest::getCookie(const std::string &key,
                                   const std::string &def) {
  XX(m_cookies);
}
#undef XX

#define XX(var) var[key] = val;
void HttpRequest::setHeader(const std::string &key, const std::string &val) {
  XX(m_headers);
}
void HttpRequest::setParam(const std::string &key, const std::string &val) {
  XX(m_params);
}
void HttpRequest::setCookie(const std::string &key, const std::string &val) {
  XX(m_cookies);
}
#undef XX

#define XX(var)                                                                \
  auto it = var.find(key);                                                     \
  if (it == var.end()) {                                                       \
    return false;                                                              \
  }                                                                            \
  if (val) {                                                                   \
    *val = it->second;                                                         \
  }                                                                            \
  return true;
bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
  XX(m_headers);
}
bool HttpRequest::hasParam(const std::string &key, std::string *val) {
  XX(m_params);
}
bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
  XX(m_cookies);
}
#undef XX

#define XX(var) var.erase(key);
void HttpRequest::delHeader(const std::string &key) { XX(m_headers); }
void HttpRequest::delParam(const std::string &key) { XX(m_params); }
void HttpRequest::delCookie(const std::string &key) { XX(m_cookies); }
#undef XX

std::ostream &HttpRequest::dump(std::ostream &os) const {
  // GET /uri HTTP/1.1
  // Host: www.xxx.com
  os << HttpMethodToString(m_method) << " " << m_path
     << (m_query.empty() ? "" : "?") << m_query
     << (m_fragment.empty() ? "" : "#") << m_fragment << " HTTP/"
     << ((uint32_t)(m_version >> 4)) << "." << ((uint32_t)(m_version & 0x0F))
     << "\r\n";

  os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

  for (auto &i : m_headers) {
    if (strcasecmp(i.first.c_str(), "connection") == 0) {
      continue;
    }
    os << i.first << ":" << i.second << "\r\n";
  }
  if (!m_body.empty()) {
    os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
  } else {
    os << "\r\n" << m_body;
  }
  return os;
}

std::string HttpRequest::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    : m_status(HttpStatus::OK), m_version(version), m_close(close) {}

std::string HttpResponse::getHeader(const std::string &key,
                                    const std::string &def) const {
  auto it = m_headers.find(key);
  return it == m_headers.end() ? def : it->second;
}
void HttpResponse::setHeader(const std::string &key, const std::string &val) {
  m_headers[key] = val;
}
void HttpResponse::delHeader(const std::string &key) { m_headers.erase(key); }

std::ostream &HttpResponse::dump(std::ostream &os) const {
  os << "HTTP/" << ((uint32_t)m_version >> 4) << "."
     << ((uint32_t)(m_version & 0x0F)) << " " << (uint32_t)m_status << " "
     << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason) << "\r\n";

  for (auto &i : m_headers) {
    if (strcasecmp(i.first.c_str(), "connection") == 0) {
      continue;
    }
    os << i.first << ": " << i.second << "\r\n";
  }
  os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
  if (!m_body.empty()) {
    os << "content-length: " << m_body.size() << "\r\n\r\n";
  } else {
    os << "\r\n\r\n";
  }
  return os;
}

std::string HttpResponse::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

} // namespace http
} // namespace apexstorm