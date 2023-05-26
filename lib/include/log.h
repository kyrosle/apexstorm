/**
 * @file log.h
 * @brief Logger module
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
 *
 * @par modified log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2023-05-25 <td>1.0     <td>wangh     <td>内容
 * </table>
 */

#ifndef __APEXSTORM_LOG_H__
#define __APEXSTORM_LOG_H__

#include "singleton.h"
#include "util.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#define APEXSTORM_LOG_LEVEL(logger, level)                                     \
  if (logger->getLevel() <= level)                                             \
  apexstorm::LogEventWrap(                                                     \
      apexstorm::LogEvent::ptr(new apexstorm::LogEvent(                        \
          logger, level, __FILE__, __LINE__, 0, apexstorm::GetThreadId(),      \
          apexstorm::GetFiberId(), time(0))))                                  \
      .getSS()

#define APEXSTORM_LOG_DEBUG(logger)                                            \
  APEXSTORM_LOG_LEVEL(logger, apexstorm::LogLevel::Level::DEBUG)
#define APEXSTORM_LOG_INFO(logger)                                             \
  APEXSTORM_LOG_LEVEL(logger, apexstorm::LogLevel::Level::INFO)
#define APEXSTORM_LOG_WARN(logger)                                             \
  APEXSTORM_LOG_LEVEL(logger, apexstorm::LogLevel::Level::WARN)
#define APEXSTORM_LOG_ERROR(logger)                                            \
  APEXSTORM_LOG_LEVEL(logger, apexstorm::LogLevel::Level::ERROR)
#define APEXSTORM_LOG_FATAL(logger)                                            \
  APEXSTORM_LOG_LEVEL(logger, apexstorm::LogLevel::Level::FATAL)

#define APEXSTORM_LOG_FMT_LEVEL(logger, level, fmt, ...)                       \
  if (logger->getLevel() <= level)                                             \
  apexstorm::LogEventWrap(                                                     \
      apexstorm::LogEvent::ptr(new apexstorm::LogEvent(                        \
          logger, level, __FILE__, __LINE__, 0, apexstorm::GetThreadId(),      \
          apexstorm::GetFiberId(), time(0))))                                  \
      .getEvent()                                                              \
      ->format(fmt, __VA_ARGS__)

#define APEXSTORM_LOG_FMT_DEBUG(logger, fmt, ...)                              \
  APEXSTORM_LOG_FMT_LEVEL(logger, apexstorm::LogLevel::Level::DEBUG, fmt,      \
                          __VA_ARGS__)
#define APEXSTORM_LOG_FMT_INFO(logger, fmt, ...)                               \
  APEXSTORM_LOG_FMT_LEVEL(logger, apexstorm::LogLevel::Level::INFO, fmt,       \
                          __VA_ARGS__)
#define APEXSTORM_LOG_FMT_WARN(logger, fmt, ...)                               \
  APEXSTORM_LOG_FMT_LEVEL(logger, apexstorm::LogLevel::Level::WARN, fmt,       \
                          __VA_ARGS__)
#define APEXSTORM_LOG_FMT_ERROR(logger, fmt, ...)                              \
  APEXSTORM_LOG_FMT_LEVEL(logger, apexstorm::LogLevel::Level::ERROR, fmt,      \
                          __VA_ARGS__)
#define APEXSTORM_LOG_FMT_FATAL(logger, fmt, ...)                              \
  APEXSTORM_LOG_FMT_LEVEL(logger, apexstorm::LogLevel::Level::FATAL, fmt,      \
                          __VA_ARGS__)

namespace apexstorm {

class Logger;

/**
 * @brief Log level
 */
class LogLevel {
public:
  enum class Level : char {
    /// Debug level
    DEBUG,
    /// Info level
    INFO,
    /// Warn level
    WARN,
    /// Error level
    ERROR,
    /// Fatal level
    FATAL,
  };
  /**
   * @brief Transfer LogLevel into string
   * @param  level      specifed LogLevel type
   * @return LogLevel context
   */
  static const char *ToString(LogLevel::Level level);

  /**
   * @brief Transfer string into LogLevel
   * @param  str        LogLevel context
   * @return LogLevel type
   */
  static LogLevel FromString(const std::string str);
};

/**
 * @brief Log event
 */
class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;

  /**
   * @brief constructor
   * @param logger          Logger
   * @param level           Logger level
   * @param file            Logger file name
   * @param line            Logger file line number
   * @param elapse          Time (milliseconds) for program startup
   * dependencies
   * @param thread_id       Thread id
   * @param fiber_id        Fiber id
   * @param time            Time (seconds) Log event
   * @param thread_name     Thread name
   */
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, int32_t m_line, uint32_t elapse,
           uint32_t thread_id, uint32_t fiber_id, uint64_t time);

  /**
   * @brief Get the Logger file
   * @return const char*
   */
  const char *getFile() const { return m_file; }
  /**
   * @brief Get the file line number
   * @return int32_t
   */
  int32_t getLine() const { return m_line; }
  /**
   * @brief Get the Time (milliseconds) for program startup dependencies
   * @return uint32_t
   */
  uint32_t getElapse() const { return m_elapse; }
  /**
   * @brief Get the Thread Id
   * @return uint32_t
   */
  uint32_t getThreadId() const { return m_threadId; }
  /**
   * @brief Get the Fiber Id
   * @return uint32_t
   */
  uint32_t getFiberId() const { return m_fiberId; }
  /**
   * @brief Get the Time (seconds) Log event
   * @return uint64_t
   */
  uint64_t getTime() const { return m_time; }
  /**
   * @brief Get the Logger content
   * @return std::string
   */
  std::string getContent() const { return m_ss.str(); }

  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief Get the Logger content stream
   * @return std::stringstream&
   */
  std::stringstream &getSS() { return m_ss; }

  void format(const char *fmt, ...);
  void format(const char *fmt, va_list al);

private:
  const char *m_file = nullptr; // file name
  int32_t m_line = 0;           // file line number
  uint32_t m_elapse = 0; // The number of milliseconds since the program started
  uint32_t m_threadId = 0; // thread id
  uint32_t m_fiberId = 0;  // fiber id
  uint64_t m_time;         // time stamp
  std::stringstream m_ss;  // content

  std::shared_ptr<Logger> m_logger;
  LogLevel::Level m_level;
};

class LogEventWrap {
public:
  LogEventWrap(LogEvent::ptr e);
  ~LogEventWrap();

  LogEvent::ptr getEvent() const { return m_event; }

  std::stringstream &getSS();

private:
  LogEvent::ptr m_event;
};

/**
 * @brief Log Formater
 */
class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;
  /**
   * @brief constructor
   * @param[in] pattern format pattern
   * @details
   *  %m message
   *  %p log level
   *  %r cumulative milliseconds
   *  %c log name
   *  %t thread id
   *  %n new line
   *  %d time
   *  %f file name
   *  %l line number
   *  %T \t
   *  %F fiber id
   *  %N thread name
   *
   *  default format:
   * "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   */
  LogFormatter(const std::string &pattern);

  /**
   * @brief  Returns formatted log text
   * @param  logger           Logger
   * @param  level            Log level
   * @param  event            Log event
   * @return std::string log text
   */
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);

public:
  /**
   * @brief Format log information
   */
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    FormatItem(const std::string &fmt = ""){};
    /**
     * @brief Destroy the Format Item object
     */
    virtual ~FormatItem() {}

    /**
     * @brief  Format log into stream
     * @param  os             Specifed output stream
     * @param  logger         Logger
     * @param  level          Log level
     * @param  event          Log event
     */
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };

  /**
   * @brief Initialize format pattern
   */
  void init();

private:
  std::string m_pattern;                // log format pattern
  std::vector<FormatItem::ptr> m_items; // log format after parsing format
};

/**
 * @brief  Log output target
 */
class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;

  /**
   * @brief Destroy the Log Appender object
   */
  virtual ~LogAppender() {}

  /**
   * @brief  Write log
   * @param  logger           Logger
   * @param  level            Log level
   * @param  event            log event
   */
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;

  /**
   * @brief Set the Formatter object
   * @param  val              specifies LogFormatter
   */
  void setFormatter(LogFormatter::ptr val) { m_formatter = val; }

  /**
   * @brief Get the Formatter object
   * @return LogFormatter::ptr current LogFormatter
   */
  LogFormatter::ptr getFormatter() { return m_formatter; }

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level level) { m_level = level; }

protected:
  LogLevel::Level m_level = LogLevel::Level::DEBUG; // log level
  LogFormatter::ptr m_formatter;                    // log formater
};

/**
 * @brief Logger
 */
class Logger : public std::enable_shared_from_this<Logger> {
public:
  typedef std::shared_ptr<Logger> ptr;

  /**
   * @brief Construct a new Logger object
   * @param  name             Logger name
   */
  Logger(const std::string &name = "root");

  /**
   * @brief Write Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void log(LogLevel::Level level, LogEvent::ptr event);

  /**
   * @brief Write DEBUG Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void debug(LogEvent::ptr event);
  /**
   * @brief Write INFO Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void info(LogEvent::ptr event);
  /**
   * @brief Write WARN Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void warn(LogEvent::ptr event);
  /**
   * @brief Write ERROR Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void error(LogEvent::ptr event);
  /**
   * @brief Write FATAL Log
   * @param  level            Log level
   * @param  event            Log event
   */
  void fatal(LogEvent::ptr event);

  /**
   * @brief Add Log target
   * @param  appender         Log target
   */
  void addAppender(LogAppender::ptr appender);
  /**
   * @brief Remove Log target
   * @param  appender         Log target
   */
  void delAppender(LogAppender::ptr appender);

  /**
   * @brief Get the Logger Level
   * @return LogLevel::Level
   */
  LogLevel::Level getLevel() const { return m_level; }
  /**
   * @brief Set the Logger Level
   * @param  level            Specifies Log level
   */
  void setLevel(LogLevel::Level level) { m_level = level; }

  /**
   * @brief Get the Logger name
   * @return const std::string&
   */
  const std::string &getName() const { return m_name; }

private:
  std::string m_name;                      // log name
  LogLevel::Level m_level;                 // log level
  std::list<LogAppender::ptr> m_appenders; // Appender collection
  LogFormatter::ptr m_formatter;           // Formatter
};

/**
 * @brief Ouput the log to console
 */
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;
};

/**
 * @brief Output the log to file
 */
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;

  FileLogAppender(const std::string &filename);
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;

  // Reopen the file, if open successfully return true.
  bool reopen();

private:
  std::string m_filename;     // file path
  std::ofstream m_filestream; // file stream
};

class LoggerManager {
public:
  LoggerManager();
  Logger::ptr getLogger(const std::string &name);

  void init();

private:
  Logger::ptr m_root;
  std::map<std::string, Logger::ptr> m_loggers;
};

typedef apexstorm::Singleton<LoggerManager> loggerMgr;

} // namespace apexstorm

#endif
