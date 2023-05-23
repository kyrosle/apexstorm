#ifndef __APEXSTORM_LOG_H__
#define __APEXSTORM_LOG_H__

#include <cstdint>
#include <fstream>
#include <list>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

namespace apexstorm {

class Logger;

class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent();

  const char *getFile() const { return m_file; }
  int32_t getLine() const { return m_line; }
  uint32_t getElapse() const { return m_elapse; }
  uint32_t getThreadId() const { return m_threadId; }
  uint32_t getFiberId() const { return m_fiberId; }
  uint64_t getTime() const { return m_time; }
  const std::string &getContent() const { return m_content; }

private:
  const char *m_file = nullptr; // file name
  int32_t m_line = 0;           // line number
  uint32_t m_elapse = 0; // The number of milliseconds since the program started
  uint32_t m_threadId = 0; // thread id
  uint32_t m_fiberId = 0;  // fiber id
  uint64_t m_time;         // time stamp
  std::string m_content;
};

class LogLevel {
public:
  enum class Level : char {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
  };
  static const char *ToString(LogLevel::Level level);
};

// Log Formatter
class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;
  LogFormatter(const std::string &pattern);

  // format:
  // %t  %thread_id $m$n
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);

public:
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    FormatItem(const std::string &fmt = ""){};
    virtual ~FormatItem() {}
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };
  void init();

private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
};

// logger output
class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {}

  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;

  void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
  LogFormatter::ptr getFormatter() { return m_formatter; }

protected:
  LogLevel::Level m_level;
  LogFormatter::ptr m_formatter;
};

// Logger
class Logger : public std::enable_shared_from_this<Logger> {
public:
  typedef std::shared_ptr<Logger> ptr;

  Logger(const std::string &name = "root");
  void log(LogLevel::Level level, LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level level) { m_level = level; }

  const std::string &getName() const { return m_name; }

private:
  std::string m_name;                      // log name
  LogLevel::Level m_level;                 // log level
  std::list<LogAppender::ptr> m_appenders; // Appender collection
};

// Format output to console
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;
};

// Format output to file
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;

  FileLogAppender(const std::string &filename);
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;

  // Reopen the file, if open successfully return true.
  bool reopen();

private:
  std::string m_filename;
  std::ofstream m_filestream;
};

} // namespace apexstorm
#endif
