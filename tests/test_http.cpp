#include "http/http.h"

void test_request() {
  apexstorm::http::HttpRequest::ptr req(new apexstorm::http::HttpRequest);
  req->setHeader("host", "www.baidu.com");
  req->setBody("hello world");

  req->dump(std::cout) << std::endl;
}

void test_response() {
  apexstorm::http::HttpResponse::ptr rsp(new apexstorm::http::HttpResponse);
  rsp->setHeader("X-X", "kyros");
  rsp->setBody("hello world");
  rsp->setStatus(400);
  rsp->setClose(false);

  rsp->dump(std::cout) << std::endl;
}

int main() {
  test_request();
  test_response();
  return 0;
}