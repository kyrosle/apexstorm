#include "./include/log.h"
#include <cctype>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
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
  MessageFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
  LevelFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << LogLevel::ToString(level);
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
  ElapseFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
  NameFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << logger->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
  ThreadIdFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class FiberFormatItem : public LogFormatter::FormatItem {
public:
  FiberFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
  DateTimeFormatItem(const std::string &format = "%Y:%m:%d %H:%M:$S")
      : m_format(format) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getTime();
  }

private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
  FilenameFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string &fmt) : LogFormatter::FormatItem(fmt) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string &str)
      : LogFormatter::FormatItem(str), m_string(str) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger,
              LogLevel::Level level, LogEvent::ptr event) override {
    os << m_string;
  }

private:
  std::string m_string;
};
// ---- FormatItems

// Logger ----
Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::Level::DEBUG) {
  m_formatter.reset(new LogFormatter("%d [%p] %f %l %m %n"));
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
FileLogAppender::FileLogAppender(const std::string &filename) {}

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

// Log Formater types:
//
// %m -- message body
// %t -- level
// $r -- time after boost up
// $c -- log name
// %t -- thread id
// %n -- enter
// %d -- time
// %f -- file name
// %l -- line number
//
// %xxx %xxx{xxx} %%
//
// TEST: m_pattern: "%d [%p] %f %l %m %n"
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
    // current pattern is %
    if ((i + 1) < m_pattern.size()) {
      if (m_pattern[i + 1] == '%') {
        normal_str.append(1, '%');
        continue;
      }
    }

    // next pattern cursor
    size_t n = i + 1;
    // format status
    int fmt_status = 0;
    // format str beginning pos
    size_t fmt_begin = 0;

    // temporary str
    std::string str;
    // temporary format str
    std::string fmt;

    // iterate pattern [i + 1, size - 1]
    while (n < m_pattern.size()) {
      // if current pattern[n] is a whitespace
      if (!std::isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
          m_pattern[n] != '}') {
        break;
      }

      // format status transfer
      // %xxx
      if (fmt_status == 0) {
        if (m_pattern[n] == '{') {
          str = m_pattern.substr(i + 1, n - i - 1);
          fmt_status = 1; // parse format
          fmt_begin = n;
          ++n;
          continue;
        }
      }
      // %{xxx}
      if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 2;
          break;
        }
      }
      ++n;
    }

    if (fmt_status == 0) {
      if (!normal_str.empty()) {
        vec.push_back(std::make_tuple(normal_str, std::string(), 0));
        normal_str.clear();
      }
      str = m_pattern.substr(i + 1, n - i - 1);
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n;
    } else if (fmt_status == 1) {
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    } else if (fmt_status == 2) {
      if (!normal_str.empty()) {
        vec.push_back(std::make_tuple(normal_str, "", 0));
        normal_str.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n;
    }
  }

  if (!normal_str.empty()) {
    vec.push_back(std::make_tuple(normal_str, "", 0));
  }

  // %m -- message body
  // %t -- level
  // $r -- time after boost up
  // $c -- log name
  // %t -- thread id
  // %n -- enter
  // %d -- time
  // %f -- file name
  // %l -- line number

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
          XX(l, LineFormatItem),
#undef XX
      };

  for (auto &i : vec) {
    if (std::get<2>(i) == 0) {
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end()) {
        m_items.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
      } else {
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }
    std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - ("
              << std::get<2>(i) << ")" << std::endl;
  }
}

// LogFormatter ----

// ---- LogEvent
LogEvent::LogEvent(const char *file, int32_t m_line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file), m_line(m_line), m_elapse(elapse), m_fiberId(fiber_id),
      m_threadId(thread_id), m_time(time) {}
// LogEvent ----

} // namespace apexstorm