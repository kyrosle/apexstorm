/**
 * @file log.h
 * @brief logger module.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_LOG_H__
#define __APEXSTORM_LOG_H__

#include "singleton.h"
#include "thread.h"
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

// used for testing the lock effect comparing with no mutex lock.
// #define TEST_ENABLE_NULL_MUTEX

// -- Use streaming to write log level logs to logger --
#define APEXSTORM_LOG_LEVEL(logger, level)                                     \
  if (logger->getLevel() <= level)                                             \
  apexstorm::LogEventWrap(                                                     \
      apexstorm::LogEvent::ptr(new apexstorm::LogEvent(                        \
          logger, level, __FILE__, __LINE__, 0, apexstorm::GetThreadId(),      \
          apexstorm::GetFiberId(), time(0), apexstorm::Thread::GetName())))    \
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
          apexstorm::GetFiberId(), time(0), apexstorm::Thread::GetName())))    \
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

// get the root logger
#define APEXSTORM_LOG_ROOT() apexstorm::LoggerMgr::GetInstance()->getRoot()

// get the logger through logger name
#define APEXSTORM_LOG_NAME(name)                                               \
  apexstorm::LoggerMgr::GetInstance()->getLogger(name)

namespace apexstorm {

class Logger;
class LoggerManager;

// Log Level.
class LogLevel {
public:
  enum class Level : char {
    // Unknown level
    UNKNOWN,
    // Debug level
    DEBUG,
    // Info level
    INFO,
    // Warn level
    WARN,
    // Error level
    ERROR,
    // Fatal level
    FATAL,
  };

  // Convert log levels to text output.
  static const char *ToString(LogLevel::Level level);

  // Convert text to log level.
  static LogLevel::Level FromString(const std::string &str);
};

// Log Event.
class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;

  // Constructor
  // @param logger          Logger
  // @param level           Logger level
  // @param file            Logger file name
  // @param line            Logger file line number
  // @param elapse          Time (milliseconds) for program startup
  // @param thread_id       Thread id
  // @param fiber_id        Fiber id
  // @param time            Time (seconds) Log event
  // @param thread_name     Thread name
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, int32_t m_line, uint32_t elapse,
           uint32_t thread_id, uint32_t fiber_id, uint64_t time,
           const std::string &thread_name);

  // Get the Logger file name.
  const char *getFile() const { return m_file; }

  // Get the file line number.
  int32_t getLine() const { return m_line; }

  // Get the Time (milliseconds) for program startup dependencies.
  uint32_t getElapse() const { return m_elapse; }

  // Get the Thread Id.
  uint32_t getThreadId() const { return m_threadId; }

  // Get the Fiber Id.
  uint32_t getFiberId() const { return m_fiberId; }

  // Get the Thread Name.
  std::string getThreadName() const { return m_threadname; }

  // Get the Time (seconds) Log event.
  uint64_t getTime() const { return m_time; }

  // Get the Logger content.
  std::string getContent() const { return m_ss.str(); }

  // Get the Logger.
  std::shared_ptr<Logger> getLogger() const { return m_logger; }

  // Get the Logger Level.
  LogLevel::Level getLevel() const { return m_level; }

  // Get the Logger content stream.
  std::stringstream &getSS() { return m_ss; }

  // Format write log content.
  void format(const char *fmt, ...);

  // Format write log content.
  void format(const char *fmt, va_list al);

private:
  // file name
  const char *m_file = nullptr;
  // file line number
  int32_t m_line = 0;
  // the number of milliseconds since the program started
  uint32_t m_elapse = 0;
  // thread id
  uint32_t m_threadId = 0;
  // fiber id
  uint32_t m_fiberId = 0;
  // time stamp
  uint64_t m_time = 0;
  // thread name
  std::string m_threadname;
  // log content flow
  std::stringstream m_ss;

  // logger
  std::shared_ptr<Logger> m_logger;
  // logger Level
  LogLevel::Level m_level;
};

// LogEvent Wrapper.
class LogEventWrap {
public:
  // Construct a new Log Event Wrap.
  LogEventWrap(LogEvent::ptr e);

  // Destroy the Log Event Wrap.
  ~LogEventWrap();

  // Get the Event.
  LogEvent::ptr getEvent() const { return m_event; }

  // Get the logger content stream.
  std::stringstream &getSS();

private:
  // log event pointer
  LogEvent::ptr m_event;
};

// Log Formater.
class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;

  // Constructor
  // @param pattern format pattern
  // @details
  //  %m message
  //  %p log level
  //  %r cumulative milliseconds
  //  %c log name
  //  %t thread id
  //  %n new line
  //  %d time
  //  %f file name
  //  %l line number
  //  %T \t
  //  %F fiber id
  //  %N thread name
  //
  //  default format:
  // "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
  LogFormatter(const std::string &pattern);

  //  Returns formatted log text.
  // @param  logger           Logger
  // @param  level            Log level
  // @param  event            Log event
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);
  std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger,
                       LogLevel::Level level, LogEvent::ptr event);

public:
  // Log content item formatting.

  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;

    // Destroy the Format Item object.
    virtual ~FormatItem() {}

    // Format log into stream.
    // @param os Specifed output stream
    // @param logger Logger
    // @param level Log level
    // @param event Log event
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };

  // Initialize, parse log patterns.
  void init();

  // Return the Format whether successfully parsed.
  bool isError() { return m_error; }

  // Return the format pattern.
  const std::string getPattern() { return m_pattern; }

private:
  // log format pattern
  std::string m_pattern;
  // log Format After Parsing Format
  std::vector<FormatItem::ptr> m_items;
  // whether parsing result is error
  bool m_error = false;
};

//  Log output target, using spin mutex lock.
class LogAppender {
  friend class Logger;

public:
  typedef std::shared_ptr<LogAppender> ptr;

#ifdef TEST_ENABLE_NULL_MUTEX
  typedef NullMutex MutexType;
#else
  typedef SpinLock MutexType;
#endif

  // Destroy the Log Appender object.
  virtual ~LogAppender() {}

  // Write log, (thread safety) implement by subclass.
  // @param logger Logger
  // @param level Log level
  // @param event log event
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;

  // Set the Formatter(thread safety).
  void setFormatter(LogFormatter::ptr val);

  // Get the Formatter object(thread safety).
  LogFormatter::ptr getFormatter();

  // Get the Log Level.
  LogLevel::Level getLevel() const { return m_level; }

  // Set the Log Level
  void setLevel(LogLevel::Level level) { m_level = level; }

  // Convert LogAppender to YAML string (thread safety).
  virtual std::string toYamlString() = 0;

protected:
  // log level
  LogLevel::Level m_level = LogLevel::Level::DEBUG;
  // log formater
  LogFormatter::ptr m_formatter;
  // dose appender have formatter
  bool m_hasFormatter = false;

  // spin mutex lock.
  MutexType m_mutex;
};

// logger, using spin mutex lock.
class Logger : public std::enable_shared_from_this<Logger> {
  friend class LoggerManager;

public:
  typedef std::shared_ptr<Logger> ptr;

#ifdef TEST_ENABLE_NULL_MUTEX
  typedef NullMutex MutexType;
#else
  typedef SpinLock MutexType;
#endif

  // Construct a new Logger object.
  // @param  name             Logger name
  Logger(const std::string &name = "root");

  // Write Log (thread safety).
  void log(LogLevel::Level level, LogEvent::ptr event);

  // Write DEBUG Log (thread safety).
  void debug(LogEvent::ptr event);

  // Write INFO Log (thread safety).
  void info(LogEvent::ptr event);

  // Write WARN Log (thread safety).
  void warn(LogEvent::ptr event);

  // Write ERROR Log (thread safety).
  void error(LogEvent::ptr event);

  // Write FATAL Log (thread safety).
  void fatal(LogEvent::ptr event);

  // Add Log target (thread safety).
  // @param  appender         Log target
  void addAppender(LogAppender::ptr appender);

  // Remove Log target (thread safety).
  // @param  appender         Log target
  void delAppender(LogAppender::ptr appender);

  // Clear all log targets (thread safety).
  void clearAppenders();

  // Get the Logger Level.
  LogLevel::Level getLevel() const { return m_level; }

  // Set the Logger Level.
  void setLevel(LogLevel::Level level) { m_level = level; }

  // Get the Logger name.
  const std::string &getName() const { return m_name; }

  // Set the Formatter, also setting status: has_formater.
  void setFormatter(LogFormatter::ptr val);
  void setFormatter(const std::string &val); // (thread safety)

  // Get the Formatter.
  LogFormatter::ptr getFormatter();

  // Convert Logger into YAML string (thread safety).
  std::string toYamlString();

private:
  // log name
  std::string m_name;
  // log level
  LogLevel::Level m_level;
  // log targets collection
  std::list<LogAppender::ptr> m_appenders;
  // log formater (unused)
  LogFormatter::ptr m_formatter;
  // refer to root logger
  Logger::ptr m_root;
  // spin mutex lock
  MutexType m_mutex;
};

// Appender output to console, using spin mutex lock.
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  // (thread safety)
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;

  // (thread safety)
  std::string toYamlString() override;
};

// Appender output to file, using spin mutex lock.
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;

  FileLogAppender(const std::string &filename);
  // (thread safety)
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;

  // Reopen the log file (thread safety).

  bool reopen();

  // (thread safety)
  std::string toYamlString() override;

private:
  // file path
  std::string m_filename;
  // file stream
  std::ofstream m_filestream;
  // recording the last time using the file log appender
  uint64_t m_lastTime = 0;
};

//  Logger Manager, using spin mutex lock.
class LoggerManager {
public:
#ifdef TEST_ENABLE_NULL_MUTEX
  typedef NullMutex MutexType;
#else
  typedef SpinLock MutexType;
#endif

  // Construct a new Logger Manager (thread safety).
  LoggerManager();

  // Get the Logger (thread safety).
  // @param  name             Specifed logger name
  Logger::ptr getLogger(const std::string &name);

  // Initialise
  void init();

  // Get the root logger.
  Logger::ptr getRoot() const { return m_root; }

  // convert all loggers into YAML string (thread safety).
  std::string toYamlString();

private:
  // Primary logger
  Logger::ptr m_root;
  // logger container
  std::map<std::string, Logger::ptr> m_loggers;
  // spin mutex lock
  MutexType m_mutex;
};

// Logger management class singleton pattern
typedef apexstorm::Singleton<LoggerManager> LoggerMgr;

} // namespace apexstorm

#endif
