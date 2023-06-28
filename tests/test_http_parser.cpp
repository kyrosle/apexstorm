#include "apexstorm.h"
#include "http/http_parser.h"
#include "log.h"

static apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                 "Host: www.baidu.com\r\n"
                                 "Content-Length:: 10\r\n\r\n"
                                 "123456789";

void test() {
  apexstorm::http::HttpRequestParer parser;
  std::string tmp = test_request_data;
  size_t s = parser.execute(&tmp[0], tmp.size());
  APEXSTORM_LOG_ERROR(g_logger)
      << "execute rt=" << s << "has_error=" << parser.hasError()
      << " is_finished=" << parser.isFinished() << " total=" << tmp.size()
      << " content-length=" << parser.getContentLength();

  tmp.resize(tmp.size() - s);

  APEXSTORM_LOG_INFO(g_logger) << parser.getData()->toString();
}

int main() {
  test();
  return 0;
}