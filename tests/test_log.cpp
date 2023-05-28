#include "../lib/include/log.h"
#include "../lib/include/util.h"
#include <cstdint>
#include <iostream>
#include <thread>
int main() {
  apexstorm::Logger::ptr logger(new apexstorm::Logger);
  logger->addAppender(
      apexstorm::LogAppender::ptr(new apexstorm::StdoutLogAppender));

  apexstorm::FileLogAppender::ptr file_appender(
      new apexstorm::FileLogAppender("./log.txt"));
  logger->addAppender(file_appender);

  apexstorm::LogFormatter::ptr fmt(new apexstorm::LogFormatter("%d%T%m%n"));
  file_appender->setFormatter(fmt);
  file_appender->setLevel(apexstorm::LogLevel::Level::ERROR);

  APEXSTORM_LOG_DEBUG(logger) << "Test Logger DEBUG";
  APEXSTORM_LOG_INFO(logger) << "Test Logger INFO";
  APEXSTORM_LOG_WARN(logger) << "Test Logger WARN";
  APEXSTORM_LOG_ERROR(logger) << "Test Logger ERROR";
  APEXSTORM_LOG_FATAL(logger) << "Test Logger FATAL";

  APEXSTORM_LOG_FMT_DEBUG(logger, "Test Logger DEBUG %s", "!TEST!1");
  APEXSTORM_LOG_FMT_INFO(logger, "Test Logger INFO %s", "!TEST!2");
  APEXSTORM_LOG_FMT_WARN(logger, "Test Logger WARN %s", "!TEST!3");
  APEXSTORM_LOG_FMT_ERROR(logger, "Test Logger ERROR %s", "!TEST!4");
  APEXSTORM_LOG_FMT_FATAL(logger, "Test Logger FATAL %s", "!TEST!5");

  auto l = apexstorm::LoggerMgr::GetInstance()->getLogger("xx");
  APEXSTORM_LOG_INFO(l) << "xxx";
}