# ApexStorm

statement

## Development Environment

Fedora38 desktop
gcc 13.1
xmake 2.4

## Project Environment

```txt
.
├── bin         (output directory)
├── build       (build directory)
├── lib         ()
├── src         ()
├── tests       ()
├── README.md
└── xmake.lua   (xmake script)

```

## Logging System

1. log4J

```txt
    Logger (Log Type)
      |
      |------ Formatter (Log Format)
      |
    Appender (Log Output)

```

## Setting System

Config --> Yaml

yaml-cpp: [github](https://github.com/jbeder/yaml-cpp)

```bash
xrepo install yaml-cpp
```

```cpp
YAML::Node node = YAML::LoadFile(filename);
node.IsMap()
for(auto it = node.begin(); it != node.end(); ++it) {
  it->first, it->second
}

node.IsSequence()
for(size_t i = 0; i < node.size(); ++i) {

}

node.IsScalar();
```

Configuration system principle, convention is better than configuration:

```cpp
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;
```

Container deviation specialization:

- vector
- list
- set
- unordered_set
- map (key = std::string)
- unordered_map (key = std::string)

tips:

- self defined `class` should implement
  `apexstorm::LexicalCast` derivation specialization,
  then `Config` will support parsing.

- self defined `class` can be used with regular `stl`(mention above) nesting.

Details examples are in `tests/test_config.cpp`.

Setting Event mechanism:
when a setting has changed, it will notify to the corresponding code, and callback.

## Logger Settings System

```yaml
logs:
  - name: root
    level: (debug, info, warn, error, fatal)
    formatter: "%d%T%p%T%t"
    appender:
      - type: (StdoutLogAppender, FileLogAppender)
        level: (debug, ...)
        file: /logs/xxx.log
```

```cpp
  apexstorm::Logger g_logger =
      apexstorm::LoggerMgr::GetInstance()->getLogger(name);
  APEXSTORM_LOG_INFO(g_logger) << "xxx log";
```

```cpp
static Logger::ptr g_log = APEXSTORM_LOG_NAME("system);
// m_root, m_system -> m_root , when logger appenders is empty, use root to write log.
```

```cpp
// define: LogDefine LogAppenderDefine, derivation specification(LexicalCast<std::string, LogDefine>)
// implementation log settings parsing
```

```mermaid
classDiagram
  class Logger {
    - name: string
    - level: LogLevel
    - appender: std::vector&ltLogAppender&gt
    - formatter: LogFormatter

    + log()
    + debug()
    + info()
    + error()
    + fatal()
    + addAppender()
    + delAppender()
    + clearAppender()
    + setFormatter()
    + setLevel()
  }
  class LogAppender {
      - level: LogLevel
      - formatter: LogFormatter

      + log()
      + toYamlString()
  }
  class FileLogAppender {
      - file: string

      + log()
      + toYamlString()
  }
  class StdoutLogAppender {

      + log()
      + toYamlString()
  }
  class LogFormatter {
    - formatter: string
    - items: vector&ltFormatterItem&gt

    + format()
  }
  class FormatItem {
    + format()
  }
  class MessageFormatItem {
    + format()
  }
  class LevelFormatItem {
    + format()
  }
  class ElapseFormatItem {
    + format()
  }
  class NameFormatItem {
    + format()
  }
  class ThreadIdFormatItem {
    + format()
  }
  class NewLineFormatItem {
    + format()
  }
  class DateTimeFormatItem {
    + format()
  }
  class FilenameFormatItem {
    + format()
  }
  class LineFormatItem {
    + format()
  }
  class TabFormatItem {
    + format()
  }
  class FiberFormatItem {
    + format()
  }

  Logger "1" ..> "n" LogAppender
  Logger "1" ..> "n" LogFormatter
  LogAppender "1" ..> "1" LogFormatter
  FileLogAppender ..|> LogAppender
  StdoutLogAppender ..|> LogAppender
  LogFormatter "1" ..> "n" FormatItem
  MessageFormatItem --|> FormatItem
  LevelFormatItem --|> FormatItem
  ElapseFormatItem --|> FormatItem
  NameFormatItem --|> FormatItem
  ThreadIdFormatItem --|> FormatItem
  NewLineFormatItem --|> FormatItem
  DateTimeFormatItem --|> FormatItem
  FilenameFormatItem --|> FormatItem
  LineFormatItem --|> FormatItem
  TabFormatItem --|> FormatItem
  FiberFormatItem --|> FormatItem
```

```mermaid
classDiagram
  class ConfigVarBase {
    - name: string
    - description: string

    - fromString()
    - toString()
  }

  class ConfigVar {
    - val: T

    - formString()
    - toString()
    - getValue()
  }

  class Config {
    - variables: map&lt&gt

    - Lookup()
  }

  class LexicalCast {
    <<template&ltF,T&gt>>
    - operator()(F): T
  }

  class LexicalCast_Special_for_vector {
    <<template&ltT&gt>>
    - operator()(F): T
  }

  Config ..> ConfigVar
  ConfigVar --|> ConfigVarBase
```

## Thread Related Encapsulation

Thread, Mutex encapsulation:

- Mutex
- SpinLock
- CASLock
- RWMutex
- Semaphore

integrate with log

logger, logAppender

using SpinLock instead of Mutex

write file periodic reopen(should fixed).

## Coroutine Library Encapsulation

more relevant introduction: [coroutine_cpp_zh](./notes/coroutine_cpp_zh.md)

Coroutine interface

ucontext_t

```cpp
// get current context and storing in `ucp` structure.
int getcontext(ucontext_t *ucp);

// set current context to `ucp`. `ucp` should get by `getcontext` or `makecontext`.
int setcontext(const ucontext_t *ucp);

// modify `ucp` (get by `getcontext`), setting `ucp` context (func).
void makecontext(ucontext_t *ucp, void (*func)(), int argc...);

// swap current context into `ucp` and storing previous context into `old_ucp`.
int swapcontext(ucontext_t *old_ucp, const ucontext_t * ucp);
```

macro for debugging

```cpp
Fiber::GetThis()
Thread -> main_fiber <----> sub_fiber
              ^
              |
              |
              |
           sub_fiber
```

coroutine scheduler

```cpp
          1 - N         1 - M
scheduler -- --> thread -- --> fiber
1. thread pool, allocate a group of threads.
2. coroutine scheduler, assign the coroutine to the corresponding thread for execution.

N : M

m_threads
<function<void() | Fiber, thread_id>>m_fibers

schedule(func/fiber)

start()
stop()
run()

1. set current thread scheduler
2. set current thread run, fiber
3. coroutine schedule while(true)
  1. coroutine message queue whether has task?
  2. not task, execute idle.
```

## Socket Library

## Http Protocol Development

## Distributed Protocol

## Recommender System
