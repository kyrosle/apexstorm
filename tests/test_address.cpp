#include "address.h"
#include "log.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

apexstorm::Logger::ptr g_logger = APEXSTORM_LOG_ROOT();

void test() {
  std::vector<apexstorm::Address::ptr> addrs;
  APEXSTORM_LOG_INFO(g_logger) << "begin";
  bool v = apexstorm::Address::Lookup(addrs, "www.baidu.com");
  APEXSTORM_LOG_INFO(g_logger) << "end";
  if (!v) {
    APEXSTORM_LOG_ERROR(g_logger) << "lookup fail";
    return;
  }

  for (size_t i = 0; i < addrs.size(); ++i) {
    APEXSTORM_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
  }
}

void test_iface() {
  std::multimap<std::string, std::pair<apexstorm::Address::ptr, uint32_t>>
      results;

  bool v = apexstorm::Address::GetInterFaceAddresses(results);

  if (!v) {
    APEXSTORM_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
    return;
  }

  for (auto &i : results) {
    APEXSTORM_LOG_INFO(g_logger)
        << i.first << " - " << i.second.first->toString() << " - "
        << i.second.second;
  }
}

void test_ipv4() {
  // auto addr = apexstorm::IPAddress::Create("www.sylar.top");
  auto addr = apexstorm::IPAddress::Create("127.0.0.8");
  if (addr) {
    APEXSTORM_LOG_INFO(g_logger) << addr->toString();
  }
}

int main() {
  // test();
  // test_iface();
  test_ipv4();
  return 0;
}
