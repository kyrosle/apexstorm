#include "./include/log.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace apexstorm {

const char *LogLevel::ToString(LogLevel::Level level) {
  switch (level) {
#define XX(name)                                                               \
  case LogLevel::Level::name:                                                  \
    return #name;                                                              \
    break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default:
    return "UNKNOWN";
  }
}

// FormatItems ----
class MessageFormatItem : public LogFormatter::FormatItem {
public:
  MessageFormatItem(const std::string &fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
  LevelFormatItem(const std::string &fmt = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << LogLevel::ToString(level);
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
  ElapseFormatItem(const std::string &fmtj = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
  NameFormatItem(const std::string &fmt = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << logger->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
  ThreadIdFormatItem(const std::string &fmt = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class FiberFormatItem : public LogFormatter::FormatItem {
public:
  FiberFormatItem(const std::string &fmt = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
  DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
      : m_format(format) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->getTime();
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
  FilenameFormatItem(const std::string &fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string &fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string &fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string &str) : m_string(str) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << m_string;
  }

private:
  std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
  TabFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << "\t";
  }

private:
  std::string m_string;
};
// ---- FormatItems

// Logger ----
Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::Level::DEBUG) {
  m_formatter.reset(new LogFormatter(
      "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
  if (!appender->getFormatter()) {
    appender->setFormatter(m_formatter);
  }
  m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender) {
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
      break;
    }
  }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    for (auto &i : m_appenders) {
      i->log(self, level, event);
    }
  }
}
void Logger::debug(LogEvent::ptr event) { log(LogLevel::Level::DEBUG, event); }
void Logger::info(LogEvent::ptr event) { log(LogLevel::Level::INFO, event); }
void Logger::warn(LogEvent::ptr event) { log(LogLevel::Level::WARN, event); }
void Logger::error(LogEvent::ptr event) { log(LogLevel::Level::ERROR, event); }
void Logger::fatal(LogEvent::ptr event) { log(LogLevel::Level::FATAL, event); }
// ---- Logger

// ---- Appender
FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
  reopen();
}

bool FileLogAppender::reopen() {
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);
  return !!m_filestream;
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
  if (level >= m_level) {
    m_filestream << m_formatter->format(logger, level, event);
  }
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    std::cout << m_formatter->format(logger, level, event);
  }
}
// Appender ----

// ---- LogFormatter
LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {
  init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &i : m_items) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

void LogFormatter::init() {
  // (str, format, type)
  std::vector<std::tuple<std::string, std::string, int>> vec;
  // recording the normal str
  std::string normal_str;
  for (size_t i = 0; i < m_pattern.size(); ++i) {
    // current pattern is normal char
    if (m_pattern[i] != '%') {
      normal_str.append(1, m_pattern[i]);
      continue;
    }

    // pattern [0, size - 1]
    if ((i + 1) < m_pattern.size()) {
      // if current pattern and next pattern are '%', add a '%' into the
      // normal_str
      if (m_pattern[i + 1] == '%') {
        normal_str.append(1, '%');
        continue;
      }
    }

    // begin from next pattern
    size_t n = i + 1;
    // format status:
    //
    // 0: initial format status, it will check next pattern whether is '{'.
    //
    // 1: parsing format string, and storing the format char into `str`, end
    // until meeting '}'.
    //
    // 2: successful parsing the format contents. The format string is stored in
    // the `str` variable, and the parsed format is stored in the `fmt`
    // variable.
    int fmt_status = 0;
    // format str beginning pos
    size_t fmt_begin = 0;

    // %str{fmt}

    // format string
    std::string str;
    // parsing format
    std::string fmt;

    // iterate pattern [i + 1, size - 1]
    while (n < m_pattern.size()) {
      // if current pattern[n] equal: !alpha, '{', '}' and format status is in
      // inital state, just break out.
      if (!fmt_status && (!std::isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                          m_pattern[n] != '}')) {
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }

      // inital format status and meeting '{'
      if (fmt_status == 0) {
        if (m_pattern[n] == '{') {
          // extract format string
          str = m_pattern.substr(i + 1, n - i - 1);
          // std::cout << "*" << str << std::endl;
          fmt_status = 1; // transfer into parsing format
          fmt_begin = n;
          ++n;
          continue;
        }
      }
      // parsing the format and meeting '}'
      if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          // extract parsing format
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          // std::cout << "#" << fmt << std::endl;
          fmt_status = 2; // transfer into parsing success
          ++n;
          break;
        }
      }
      // advance the cursor
      ++n;
    }

    // After parsing format, check the format status.
    if (fmt_status == 0) {
      // does not meet "%m{xxx}", just like "%m"
      if (!normal_str.empty()) {
        vec.push_back(std::make_tuple(normal_str, std::string(), 0));
        normal_str.clear();
      }
      // extract the format string
      str = m_pattern.substr(i + 1, n - i - 1);
      vec.push_back(std::make_tuple(str, fmt, 1));

      // adjust the cursor
      i = n - 1;
    } else if (fmt_status == 1) {
      // still in parsing state, error.
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    } else if (fmt_status == 2) {
      // parsing format successfully.
      if (!normal_str.empty()) {
        vec.push_back(std::make_tuple(normal_str, "", 0));
        normal_str.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));

      // adjust the cursor
      i = n - 1;
    }
  }

  // handle tail normal string
  if (!normal_str.empty()) {
    vec.push_back(std::make_tuple(normal_str, "", 0));
  }

  // hashmap: <format string>(parsing format)
  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string &str)>>
      s_format_items = {
#define XX(str, C)                                                             \
  {                                                                            \
    #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }   \
  }

          XX(m, MessageFormatItem),  XX(p, LevelFormatItem),
          XX(r, ElapseFormatItem),   XX(c, NameFormatItem),
          XX(t, ThreadIdFormatItem), XX(n, NewLineFormatItem),
          XX(d, DateTimeFormatItem), XX(f, FilenameFormatItem),
          XX(l, LineFormatItem),     XX(T, TabFormatItem),
          XX(F, FiberFormatItem),
#undef XX
      };

  // iterate vector<(string       ,         string, int)>
  //                 format string, parsing string, type
  for (auto &i : vec) {
    // type == 0 ? => string item
    if (std::get<2>(i) == 0) {
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      // it = (format string)
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end()) {
        // not found format string
        m_items.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
      } else {
        // push_back func(parsing string)
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }
    // print out the parsing item list
    // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") -
    // ("
    //           << std::get<2>(i) << ")" << std::endl;
  }
  // std::cout << m_items.size() << std::endl;
}

// LogFormatter ----

// ---- LogEvent
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, int32_t m_line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(m_line), m_elapse(elapse), m_fiberId(fiber_id),
      m_threadId(thread_id), m_time(time), m_logger(logger), m_level(level) {}

void LogEvent::format(const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}

void LogEvent::format(const char *fmt, va_list al) {
  char *buf = nullptr;
  int len = vasprintf(&buf, fmt, al);
  if (len != -1) {
    m_ss << std::string(buf, len);
    free(buf);
  }
}

// LogEvent ----

// ---- LogEventWarp
LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}

LogEventWrap::~LogEventWrap() {
  m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }
// LogEventWarp ----

// ---- LogManager
LoggerManager::LoggerManager() {
  m_root.reset(new Logger);

  m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

Logger::ptr LoggerManager::getLogger(const std::string &name) {
  auto it = m_loggers.find(name);
  return it == m_loggers.end() ? m_root : it->second;
}

void LoggerManager::init() {}
// LogManager ----

} // namespace apexstorm