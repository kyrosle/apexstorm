/**
 * @file log.h
 * @brief Logger module
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
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

// -- Use streaming to write log level logs to logger --
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

// -- Write log level logs to logger using formatting --
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

#define APEXSTORM_LOG_ROOT() apexstorm::LoggerMgr::GetInstance()->getRoot()

namespace apexstorm {

class Logger;
class LoggerManager;

/**
 * @brief Log Level
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
   * @brief Convert log levels to text output
   * @param  level        Specifed LogLevel type
   */
  static const char *ToString(LogLevel::Level level);

  /**
   * @brief Convert text to log level
   * @param  str          LogLevel text
   */
  static LogLevel FromString(const std::string str);
};

/**
 * @brief Log Event
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
   * @param thread_id       Thread id
   * @param fiber_id        Fiber id
   * @param time            Time (seconds) Log event
   * @param thread_name     Thread name
   */
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, int32_t m_line, uint32_t elapse,
           uint32_t thread_id, uint32_t fiber_id, uint64_t time);

  /**
   * @brief Get the Logger file name
   */
  const char *getFile() const { return m_file; }

  /**
   * @brief Get the file line number
   */
  int32_t getLine() const { return m_line; }
  /**
   * @brief Get the Time (milliseconds) for program startup dependencies
   */
  uint32_t getElapse() const { return m_elapse; }

  /**
   * @brief Get the Thread Id
   */
  uint32_t getThreadId() const { return m_threadId; }

  /**
   * @brief Get the Fiber Id
   */
  uint32_t getFiberId() const { return m_fiberId; }

  /**
   * @brief Get the Time (seconds) Log event
   */
  uint64_t getTime() const { return m_time; }

  /**
   * @brief Get the Logger content
   */
  std::string getContent() const { return m_ss.str(); }

  /**
   * @brief Get the Logger
   */
  std::shared_ptr<Logger> getLogger() const { return m_logger; }

  /**
   * @brief Get the Logger Level
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief Get the Logger content stream
   */
  std::stringstream &getSS() { return m_ss; }

  /**
   * @brief Format write log content
   */
  void format(const char *fmt, ...);

  /**
   * @brief Format write log content
   */
  void format(const char *fmt, va_list al);

private:
  /// file name
  const char *m_file = nullptr;
  /// file line number
  int32_t m_line = 0;
  /// The number of milliseconds since the program started
  uint32_t m_elapse = 0;
  /// Thread id
  uint32_t m_threadId = 0;
  /// Fiber id
  uint32_t m_fiberId = 0;
  /// Time stamp
  uint64_t m_time = 0;
  /// log content flow
  std::stringstream m_ss;

  /// Logger
  std::shared_ptr<Logger> m_logger;
  /// Logger Level
  LogLevel::Level m_level;
};

/**
 * @brief LogEvent Wrapper
 */
class LogEventWrap {
public:
  /**
   * @brief Construct a new Log Event Wrap
   * @param  e               Logger reference
   */
  LogEventWrap(LogEvent::ptr e);

  /**
   * @brief Destroy the Log Event Wrap
   */
  ~LogEventWrap();

  /**
   * @brief Get the Event
   */
  LogEvent::ptr getEvent() const { return m_event; }

  /**
   * @brief Get the logger content stream
   */
  std::stringstream &getSS();

private:
  /// Log Event
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
   * @param pattern format pattern
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
  std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger,
                       LogLevel::Level level, LogEvent::ptr event);

public:
  /**
   * @brief Log content item formatting
   */
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
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
   * @brief Initialize, parse log patterns
   */
  void init();

private:
  /// Log format pattern
  std::string m_pattern;
  /// Log Format After Parsing Format
  std::vector<FormatItem::ptr> m_items;
  /// whether is error
  bool m_error = false;
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
   */
  LogFormatter::ptr getFormatter() { return m_formatter; }

  /**
   * @brief Get the Log Level
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief Set the Log Level
   */
  void setLevel(LogLevel::Level level) { m_level = level; }

protected:
  /// Log level
  LogLevel::Level m_level = LogLevel::Level::DEBUG;
  /// Log formater
  LogFormatter::ptr m_formatter;
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
   * @brief Clear all log targets
   */
  void clearAppenders();

  /**
   * @brief Get the Logger Level
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief Set the Logger Level
   * @param  level            Specifies Log level
   */
  void setLevel(LogLevel::Level level) { m_level = level; }

  /**
   * @brief Get the Logger name
   */
  const std::string &getName() const { return m_name; }

private:
  /// Log name
  std::string m_name;
  /// Log level
  LogLevel::Level m_level;
  /// Log targets collection
  std::list<LogAppender::ptr> m_appenders;
  /// Log formater (unused)
  LogFormatter::ptr m_formatter;
};

/**
 * @brief Appender output to console
 */
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;
};

/**
 * @brief Appender output to file
 */
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;

  FileLogAppender(const std::string &filename);
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;

  /**
   * @brief Reopen the log file
   */
  bool reopen();

private:
  /// file path
  std::string m_filename;
  /// file stream
  std::ofstream m_filestream;
};

/**
 * @brief  Logger Manager
 */
class LoggerManager {
public:
  /**
   * @brief Construct a new Logger Manager
   */
  LoggerManager();

  /**
   * @brief Get the Logger
   * @param  name             Specifed logger name
   */
  Logger::ptr getLogger(const std::string &name);

  /**
   * @brief Initialise
   */
  void init();

  Logger::ptr getRoot() const { return m_root; }

private:
  /// Primary logger
  Logger::ptr m_root;
  /// logger container
  std::map<std::string, Logger::ptr> m_loggers;
};

/// Logger management class singleton pattern
typedef apexstorm::Singleton<LoggerManager> LoggerMgr;

} // namespace apexstorm

#endif
