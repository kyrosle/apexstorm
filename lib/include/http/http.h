/**
 * @file http.h
 * @author your name (you@domain.com)
 * @brief HTTP protocol structure encapsulating.
 * @version 0.1
 * @date 2023-06-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_HTTP_H__
#define __APEXSTORM_HTTP_H__

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "log.h"
#include "util.h"

// this header should not put to the top,
// otherwise it will detected error by clangd:
// templates must have C++ linkage
#include <boost/lexical_cast.hpp>

namespace apexstorm {

namespace http {

/*
  refer to https://github.com/nodejs/http-parser/blob/main/http_parser.h
 */

/* Request Methods */
#define HTTP_METHOD_MAP(XX)                                                    \
  XX(0, DELETE, DELETE)                                                        \
  XX(1, GET, GET)                                                              \
  XX(2, HEAD, HEAD)                                                            \
  XX(3, POST, POST)                                                            \
  XX(4, PUT, PUT)                                                              \
  /* pathological */                                                           \
  XX(5, CONNECT, CONNECT)                                                      \
  XX(6, OPTIONS, OPTIONS)                                                      \
  XX(7, TRACE, TRACE)                                                          \
  /* WebDAV */                                                                 \
  XX(8, COPY, COPY)                                                            \
  XX(9, LOCK, LOCK)                                                            \
  XX(10, MKCOL, MKCOL)                                                         \
  XX(11, MOVE, MOVE)                                                           \
  XX(12, PROPFIND, PROPFIND)                                                   \
  XX(13, PROPPATCH, PROPPATCH)                                                 \
  XX(14, SEARCH, SEARCH)                                                       \
  XX(15, UNLOCK, UNLOCK)                                                       \
  XX(16, BIND, BIND)                                                           \
  XX(17, REBIND, REBIND)                                                       \
  XX(18, UNBIND, UNBIND)                                                       \
  XX(19, ACL, ACL)                                                             \
  /* subversion */                                                             \
  XX(20, REPORT, REPORT)                                                       \
  XX(21, MKACTIVITY, MKACTIVITY)                                               \
  XX(22, CHECKOUT, CHECKOUT)                                                   \
  XX(23, MERGE, MERGE)                                                         \
  /* upnp */                                                                   \
  XX(24, MSEARCH, M - SEARCH)                                                  \
  XX(25, NOTIFY, NOTIFY)                                                       \
  XX(26, SUBSCRIBE, SUBSCRIBE)                                                 \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)                                             \
  /* RFC-5789 */                                                               \
  XX(28, PATCH, PATCH)                                                         \
  XX(29, PURGE, PURGE)                                                         \
  /* CalDAV */                                                                 \
  XX(30, MKCALENDAR, MKCALENDAR)                                               \
  /* RFC-2068, section 19.6.1.2 */                                             \
  XX(31, LINK, LINK)                                                           \
  XX(32, UNLINK, UNLINK)                                                       \
  /* icecast */                                                                \
  XX(33, SOURCE, SOURCE)

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                    \
  XX(100, CONTINUE, Continue)                                                  \
  XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                            \
  XX(102, PROCESSING, Processing)                                              \
  XX(200, OK, OK)                                                              \
  XX(201, CREATED, Created)                                                    \
  XX(202, ACCEPTED, Accepted)                                                  \
  XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)      \
  XX(204, NO_CONTENT, No Content)                                              \
  XX(205, RESET_CONTENT, Reset Content)                                        \
  XX(206, PARTIAL_CONTENT, Partial Content)                                    \
  XX(207, MULTI_STATUS, Multi - Status)                                        \
  XX(208, ALREADY_REPORTED, Already Reported)                                  \
  XX(226, IM_USED, IM Used)                                                    \
  XX(300, MULTIPLE_CHOICES, Multiple Choices)                                  \
  XX(301, MOVED_PERMANENTLY, Moved Permanently)                                \
  XX(302, FOUND, Found)                                                        \
  XX(303, SEE_OTHER, See Other)                                                \
  XX(304, NOT_MODIFIED, Not Modified)                                          \
  XX(305, USE_PROXY, Use Proxy)                                                \
  XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                              \
  XX(308, PERMANENT_REDIRECT, Permanent Redirect)                              \
  XX(400, BAD_REQUEST, Bad Request)                                            \
  XX(401, UNAUTHORIZED, Unauthorized)                                          \
  XX(402, PAYMENT_REQUIRED, Payment Required)                                  \
  XX(403, FORBIDDEN, Forbidden)                                                \
  XX(404, NOT_FOUND, Not Found)                                                \
  XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                              \
  XX(406, NOT_ACCEPTABLE, Not Acceptable)                                      \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)        \
  XX(408, REQUEST_TIMEOUT, Request Timeout)                                    \
  XX(409, CONFLICT, Conflict)                                                  \
  XX(410, GONE, Gone)                                                          \
  XX(411, LENGTH_REQUIRED, Length Required)                                    \
  XX(412, PRECONDITION_FAILED, Precondition Failed)                            \
  XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                                \
  XX(414, URI_TOO_LONG, URI Too Long)                                          \
  XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                      \
  XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                        \
  XX(417, EXPECTATION_FAILED, Expectation Failed)                              \
  XX(421, MISDIRECTED_REQUEST, Misdirected Request)                            \
  XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                          \
  XX(423, LOCKED, Locked)                                                      \
  XX(424, FAILED_DEPENDENCY, Failed Dependency)                                \
  XX(426, UPGRADE_REQUIRED, Upgrade Required)                                  \
  XX(428, PRECONDITION_REQUIRED, Precondition Required)                        \
  XX(429, TOO_MANY_REQUESTS, Too Many Requests)                                \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)    \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)        \
  XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                        \
  XX(501, NOT_IMPLEMENTED, Not Implemented)                                    \
  XX(502, BAD_GATEWAY, Bad Gateway)                                            \
  XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                            \
  XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                    \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)              \
  XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                    \
  XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                          \
  XX(508, LOOP_DETECTED, Loop Detected)                                        \
  XX(510, NOT_EXTENDED, Not Extended)                                          \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

// Method Definitions
enum class HttpMethod {
#define XX(num, name, string) name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
      HIT_INVALID_METHOD = 255
};

// Status Definitions
enum class HttpStatus {
#define XX(code, name, desc) name = code,
  HTTP_STATUS_MAP(XX)
#undef XX
};

// Convert string to http-method.
HttpMethod StringToHttpMethod(const std::string &m);

// Convert chars to http-method.
HttpMethod CharsToHttpMethod(const char *m);

// Convert http-method to string.
const char *HttpMethodToString(const HttpMethod &m);

// Convert http-status to string.
const char *HttpStatusToString(const HttpStatus &s);

// Check the key exists, return the corresponding value in `val`, if it's not
// exist, return the default value(`def`) MapType(string, string),
// T(corresponding key-value type).
template <class MapType, class T>
bool checkGetAs(const MapType &m, const std::string &key, T &val,
                const T &def = T()) {
  auto it = m.find(key);
  if (it == m.end()) {
    val = def;
    return false;
  }
  try {
    val = boost::lexical_cast<T>(it->second);
    return true;
  } catch (...) {
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
        << "Failed to convert '" << *it << "' to '"
        << debug_utils::cpp_type_name<T>() << "'.";
    val = def;
  }
  return false;
}

// Return the corresponding key value, otherwise return the default value(`def`)
template <class MapType, class T>
T getAs(const MapType &m, const std::string &key, const T &def = T()) {
  auto it = m.find(key);
  if (it == m.end()) {
    return def;
  }
  try {
    return boost::lexical_cast<T>(it->second);
  } catch (...) {
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
        << "Failed to convert '" << it->second << "' to '"
        << debug_utils::cpp_type_name<T>() << "'.";
  }
  return def;
}

// Provide case insensitive comparison
struct CaseInsensitiveLess {
  bool operator()(const std::string &lhs, const std::string &rhs) const;
};

// HTTP Request structure
class HttpRequest {
public:
  typedef std::shared_ptr<HttpRequest> ptr;
  typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

  // Constructor default version is 0x11, auto close status is true.
  HttpRequest(uint8_t version = 0x11, bool close = true);

  // Get the request method.
  HttpMethod getMethod() const { return m_method; }

  // Get the request version.
  uint8_t getVersion() const { return m_version; }

  // Get the request path.
  const std::string &getPath() const { return m_path; }

  // Get the request query.
  const std::string &getQuery() const { return m_query; }

  // Get the request body.
  const std::string &getBody() const { return m_body; }

  // Get the request headers.
  const MapType getHeader() const { return m_headers; }

  // Get the request params.
  const MapType getParams() const { return m_params; }

  // Get the request cookies.
  const MapType getCookies() const { return m_cookies; }

  // Set the request method.
  void setMethod(HttpMethod v) { m_method = v; }

  // Set the request version.
  void setVersion(uint8_t v) { m_version = v; }

  // Set the request path.
  void setPath(const std::string &v) { m_path = v; }

  // Set the request query.
  void setQuery(const std::string &v) { m_query = v; }

  // Set the request fragment.
  void setFragment(const std::string &v) { m_fragment = v; }

  // Set the request body.
  void setBody(const std::string &v) { m_body = v; }

  // Whether the connection is automatic closed.
  bool isClose() const { return m_close; }

  // Set the connection whether is automatic closed.
  void setClose(bool v) { m_close = v; }

  // Get header value by header key.
  std::string getHeader(const std::string &key, const std::string &def = "");

  // Get param value by param key.
  std::string getParam(const std::string &key, const std::string &def = "");

  // Get cookie value by cookie key.
  std::string getCookie(const std::string &key, const std::string &def = "");

  // Set header key-value.
  void setHeader(const std::string &key, const std::string &val);

  // Set param key-value.
  void setParam(const std::string &key, const std::string &val);

  // Set cookie key-value.
  void setCookie(const std::string &key, const std::string &val);

  // Check whether has header key.
  bool hasHeader(const std::string &key, std::string *val = nullptr);

  // Check whether has param key.
  bool hasParam(const std::string &key, std::string *val = nullptr);

  // Check whether has cookie key.
  bool hasCookie(const std::string &key, std::string *val = nullptr);

  // Delete header key.
  void delHeader(const std::string &key);

  // Delete param key.
  void delParam(const std::string &key);

  // Delete cookie key.
  void delCookie(const std::string &key);

  // Check the header key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T()) {
    return checkGetAs(m_headers, key, val, def);
  }

  // Get the header key, and transfer it into T type, otherwise return default
  // value.
  template <class T> T getHeaderAs(const std::string &key, const T &def = T()) {
    return getAs(m_headers, key, def);
  }

  // Check the param key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  bool checkGetParamAs(const std::string &key, T &val, const T &def = T()) {
    return checkGetAs(m_params, key, val, def);
  }

  // Get the header key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  T getParamAs(const std::string &key, T &val, const T &def = T()) {
    return getAs(m_params, key, def);
  }

  // Check the cookie key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  bool checkGetCookieAs(const std::string &key, T &val, const T &def = T()) {
    return checkGetAs(m_cookies, key, val, def);
  }

  // Get the cookie key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  T getCookieAs(const std::string &key, T &val, const T &def = T()) {
    return getAs(m_cookies, key, def);
  }

  // Serialize the request and output to stream.
  std::ostream &dump(std::ostream &os) const;

  // Transfer into string.
  std::string toString() const;

private:
  // HTTP method
  HttpMethod m_method;
  // HTTP version
  uint8_t m_version;
  // whether automatic close.
  bool m_close;

  // request path.
  std::string m_path;
  // request params.
  std::string m_query;
  // request fragment.
  std::string m_fragment;
  // request message body.
  std::string m_body;

  // request headers
  MapType m_headers;
  // request params
  MapType m_params;
  // request cookies
  MapType m_cookies;
};

// HTTP Response structure
class HttpResponse {
public:
  typedef std::shared_ptr<HttpResponse> ptr;
  typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

  // Constructor default version is 0x11, auto close status is true.
  HttpResponse(uint8_t version = 0x11, bool close = true);

  // Get response status.
  HttpStatus getStatus() const { return m_status; }

  // Get response version.
  uint8_t getVersion() const { return m_version; }

  // Get response payload body.
  const std::string &getBody() const { return m_body; }

  // Get response reason.
  const std::string &getReason() const { return m_reason; }

  // Get header map.
  const MapType &getHeaders() const { return m_headers; }

  // Set response status.
  void setStatus(HttpStatus v) { m_status = v; }

  // Set response status code.
  void setStatus(int v) { m_status = (HttpStatus)v; } /* should check valid */

  // Set response version.
  void setVersion(uint8_t v) { m_version = v; }

  // Set response payload body.
  void setBody(const std::string &v) { m_body = v; }

  // Set response reason.
  void setReason(const std::string &v) { m_reason = v; }

  // Set response header map.
  void setHeaders(const MapType &v) { m_headers = v; }

  // Check whether automatically close.
  bool isClose() const { return m_close; }

  // Set whether automatically close.
  void setClose(bool v) { m_close = v; }

  // Get value by key in headers.
  std::string getHeader(const std::string &key,
                        const std::string &def = "") const;

  // Set key-value in header map.
  void setHeader(const std::string &key, const std::string &val);

  // Delete key in header map.
  void delHeader(const std::string &key);

  // Check the header key, and transfer it into T type, otherwise return default
  // value.
  template <class T>
  bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T()) {
    return checkGetAs(m_headers, key, val, def);
  }

  // Get the header key, and transfer it into T type, otherwise return default
  // value.
  template <class T> T getHeaderAs(const std::string &key, const T &def = T()) {
    return getAs(m_headers, key, def);
  }

  // Serialize the request and output to stream.
  std::ostream &dump(std::ostream &os) const;

  // Transfer into string.
  std::string toString() const;

private:
  // response status
  HttpStatus m_status;
  // response version
  uint8_t m_version;
  // whether automatically close
  bool m_close;

  // response payload body
  std::string m_body;
  // response reason
  std::string m_reason;

  // response headers
  MapType m_headers;
};

} // namespace http
} // namespace apexstorm

#endif