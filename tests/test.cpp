#include "../lib/include/log.h"
#include <iostream>
int main() {
  std::cout << "Testing" << std::endl;
  apexstorm::Logger::ptr logger(new apexstorm::Logger);
  logger->addAppender(
      apexstorm::LogAppender::ptr(new apexstorm::StdoutLogAppender));

  apexstorm::LogEvent::ptr event(
      new apexstorm::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));

  logger->log(apexstorm::LogLevel::Level::DEBUG, event);
}