#include "./include/log.h"
#include "config.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include <cctype>
#include <cstddef>
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

/// LogLevel -> string
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

/// string -> LogLevel
LogLevel::Level LogLevel::FromString(const std::string &v) {
#define XX(level, str)                                                         \
  if (v == #str) {                                                             \
    return LogLevel::Level::level;                                             \
  }
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);

  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
  return LogLevel::Level::UNKNOWN;
#undef XX
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
  ElapseFormatItem(const std::string &fmt = "") {}
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
    os << event->getLogger()->getName();
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
  // if (name == "root") {
  //   m_appenders.push_back(LogAppender::ptr(new StdoutLogAppender));
  // }
}

void Logger::addAppender(LogAppender::ptr appender) {
  if (!appender->getFormatter()) {
    appender->m_formatter = m_formatter;
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

void Logger::clearAppenders() { m_appenders.clear(); }

void Logger::setFormatter(const std::string &val) {
  apexstorm::LogFormatter::ptr new_val(new apexstorm::LogFormatter(val));
  if (new_val->isError()) {
    std::cout << "Logger setFormatter name=" << m_name << " value=" << val
              << " invalid formatter";
    return;
  }
  setFormatter(new_val);
}

void Logger::setFormatter(LogFormatter::ptr val) {
  m_formatter = val;

  // if its appender doesn't have specified formatter,
  // push down the modify formatter.
  for (auto &i : m_appenders) {
    if (!i->m_hasFormatter) {
      i->m_formatter = m_formatter;
    }
  }
}

LogFormatter::ptr Logger::getFormatter() { return m_formatter; }

std::string Logger::toYamlString() {
  YAML::Node node;
  node["name"] = m_name;
  if (m_level != LogLevel::Level::UNKNOWN) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }

  for (auto &i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    if (!m_appenders.empty()) {
      for (auto &i : m_appenders) {
        i->log(self, level, event);
      }
    } else if (m_root) {
      m_root->log(level, event);
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
void LogAppender::setFormatter(LogFormatter::ptr val) {
  m_formatter = val;
  if (m_formatter) {
    m_hasFormatter = true;
  } else {
    m_hasFormatter = false;
  }
}

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

std::string FileLogAppender::toYamlString() {
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;

  if (m_level != LogLevel::Level::UNKNOWN) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_formatter && m_hasFormatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    std::cout << m_formatter->format(logger, level, event);
  }
}

std::string StdoutLogAppender::toYamlString() {
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  if (m_level != LogLevel::Level::UNKNOWN) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_formatter && m_hasFormatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
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
      m_error = true;
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
        m_error = true;
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
  // al: Address of the first mutable parameter in the parameter list.
  // fmt: Address of the last fixed parameter in the variable parameter
  // list(Stack access, push the stack from right to left).
  va_start(al, fmt);
  format(fmt, al);
  // free al
  va_end(al);
}

void LogEvent::format(const char *fmt, va_list al) {
  char *buf = nullptr;
  // `vasprintf` automatically allocates memory large enough to store the
  // formatted string.
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

  m_loggers[m_root->m_name] = m_root;

  init();
}

Logger::ptr LoggerManager::getLogger(const std::string &name) {
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }
  Logger::ptr logger(new Logger(name));
  logger->m_root = m_root;
  m_loggers[name] = logger;
  return logger;
}

std::string LoggerManager::toYamlString() {
  YAML::Node node;
  for (auto &i : m_loggers) {
    node.push_back(YAML::Load(i.second->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

// LogManager ----
struct LogAppenderDefine {
  int type = 0; // 1 File, 2 Stdout
  LogLevel::Level level = LogLevel::Level::UNKNOWN;
  std::string formatter;
  std::string file;

  bool operator==(const LogAppenderDefine &oth) const {
    return type == oth.type && level == oth.level &&
           formatter == oth.formatter && file == oth.file;
  }
};

struct LogDefine {
  std::string name;
  LogLevel::Level level = LogLevel::Level::UNKNOWN;
  std::string formatter;

  std::vector<LogAppenderDefine> appenders;

  bool operator==(const LogDefine &oth) const {
    return name == oth.name && level == oth.level &&
           formatter == oth.formatter && appenders == oth.appenders;
  }

  bool operator<(const LogDefine &oth) const { return name < oth.name; }
};

// serialization and deserialization `LogDefine`
template <> class LexicalCast<std::string, std::set<LogDefine>> {
public:
  std::set<LogDefine> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::set<LogDefine> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      const auto &n = node[i];
      // name field must exist!
      if (!n["name"].IsDefined()) {
        std::cout << "log config error: name is null, " << n << std::endl;
        continue;
      }

      // start parsing log define.
      LogDefine ld;

      // convert `name`
      ld.name = n["name"].as<std::string>();

      // convert `level`, UNKNOWN as default.
      ld.level = LogLevel::FromString(
          n["level"].IsDefined() ? n["level"].as<std::string>() : "");

      // convert `formatter` if defined.
      if (n["formatter"].IsDefined()) {
        ld.formatter = n["formatter"].as<std::string>();
      }

      // convert `appenders` if defined.
      if (n["appenders"].IsDefined()) {
        // staring parsing `appender` sequence.
        for (size_t x = 0; x < n["appenders"].size(); ++x) {
          // convert `appenders`[x].
          auto a = n["appenders"][x];

          // switch appender type (1, 2, error) and converting it.
          // type field must exist!
          if (!a["type"].IsDefined()) {
            std::cout << "log config error: appender type is null, " << a
                      << std::endl;
            continue;
          }
          std::string type = a["type"].as<std::string>();
          // appender
          LogAppenderDefine lad;
          if (type == "FileLogAppender") {
            // + file field
            lad.type = 1;
            if (!a["file"].IsDefined()) {
              std::cout << "log config error: file is null, " << a << std::endl;
              continue;
            }
            lad.file = a["file"].as<std::string>();

            // convert appender formatter
            if (a["formatter"].IsDefined()) {
              lad.formatter = a["formatter"].as<std::string>();
            }
          } else if (type == "StdoutLogAppender") {
            lad.type = 2;
          } else {
            std::cout << "log config error: appender type is invalid, " << a
                      << std::endl;
            continue;
          }

          // append appender to node.appenders
          ld.appenders.push_back(lad);
        }
      }

      // insert node to set collection
      vec.insert(ld);
    }
    return vec;
  }
};

template <> class LexicalCast<std::set<LogDefine>, std::string> {
public:
  std::string operator()(const std::set<LogDefine> &v) {
    YAML::Node node;
    for (auto &i : v) {
      YAML::Node n;
      n["name"] = i.name;
      if (i.level == LogLevel::Level::UNKNOWN) {
        n["level"] = LogLevel::ToString(i.level);
      }
      if (!i.formatter.empty()) {
        n["formatter"] = i.formatter;
      }

      for (auto &a : i.appenders) {
        YAML::Node na;

        if (a.type == 1) {
          na["type"] = "FileLogAppender";
          na["file"] = a.file;
        } else if (a.type == 2) {
          na["type"] = "StdoutLogAppender";
        }

        if (a.level != LogLevel::Level::UNKNOWN) {
          na["level"] = LogLevel::ToString(a.level);
        }

        if (a.formatter.empty()) {
          na["formatter"] = a.formatter;
        }

        n["appenders"].push_back(na);
      }

      node.push_back(n);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// initialize log config
apexstorm::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    apexstorm::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

// Initialize the before main()
struct LogIniter {
  LogIniter() {
    g_log_defines->addListener(
        0xF1E231, [](const std::set<LogDefine> &old_value,
                     const std::set<LogDefine> &new_value) {
          APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT()) << "on_logger_conf_changed";
          // add
          for (auto &i : new_value) {
            auto it = old_value.find(i);
            apexstorm::Logger::ptr logger;

            if (it == old_value.end()) {
              // add logger
              logger = APEXSTORM_LOG_NAME(i.name);
            } else {
              if (!(i == *it)) {
                // modify logger
                logger = APEXSTORM_LOG_NAME(i.name);
              }
            }

            logger->setLevel(i.level);
            if (!i.formatter.empty()) {
              logger->setFormatter(i.formatter);
            }

            logger->clearAppenders();
            for (auto &a : i.appenders) {
              apexstorm::LogAppender::ptr ap;
              if (a.type == 1) {
                // File
                ap.reset(new FileLogAppender(a.file));
              } else if (a.type == 2) {
                // Stdout
                ap.reset(new StdoutLogAppender);
              }
              ap->setLevel(a.level);
              if (!a.formatter.empty()) {
                LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                if (!fmt->isError()) {
                  ap->setFormatter(fmt);
                } else {
                  std::cout << "log.name=" << i.name
                            << "appender type=" << a.type
                            << " formatter=" << a.formatter << " is invalid"
                            << std::endl;
                }
              }
              logger->addAppender(ap);
            }
          }

          // remove
          for (auto &i : old_value) {
            auto it = new_value.find(i);
            if (it == new_value.end()) {
              // remove logger
              auto logger = APEXSTORM_LOG_NAME(i.name);
              // set Highest level
              logger->setLevel((LogLevel::Level)100);
              logger->clearAppenders();
            }
          }
        });
  }
};

static LogIniter __log_init;

void LoggerManager::init() {}
// LogManager ----

} // namespace apexstorm