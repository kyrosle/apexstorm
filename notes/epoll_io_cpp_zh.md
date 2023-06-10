# 基于 epoll 实现 异步 IO

## Brief Introduction

```cpp
class IOManager : public Scheduler {
public:
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");
  ~IOManager();
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);
  bool cancelAll(int fd);
  static IOManager *GetThis();
protected:
  void tickle() override;
  bool stopping() override;
  void idle() override;

  void contextResize(size_t size);
};

// IO 事件
enum class Event : uint32_t {
  NONE = 0x0, //  无事件
  READ = 0x1, //  读事件 (EPOLLIN)
  WRITE = 0x4 //  写事件 (EPOLLOUT)
};

// Socket 句柄上下文
struct FdContext {
  EventContext &getContext(Event event);
  void resetContext(EventContext &ctx);
  void triggerEvent(Event event);

  EventContext read;
  EventContext write;
  int fd;
  Event events = Event::NONE;
};

// 事件 上下文
struct EventContext {
  Scheduler *scheduler = nullptr;
  Fiber::ptr fiber;
  std::function<void()> cb;
};
```

epoll 相关 api [from](https://www.cnblogs.com/xuewangkai/p/11158576.html):

```cpp
// 创建一个epoll的句柄，size用来告诉内核这个监听的数目一共有多大.
epoll_create(size)
// epoll的事件注册函数, epoll_ctl向 epoll对象中添加, 修改或者删除感兴趣的事件,
// 返回0表示成功, 否则返回–1, 此时需要根据errno错误码判断错误类型.
// op:
//    EPOLL_CTL_ADD：注册新的fd到epfd中,
//    EPOLL_CTL_MOD：修改已经注册的fd的监听事件,
//    EPOLL_CTL_DEL：从epfd中删除一个fd,
// event:
//    EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）,
//    EPOLLOUT：表示对应的文件描述符可以写,
//    EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）,
//    EPOLLERR：表示对应的文件描述符发生错误,
//    EPOLLHUP：表示对应的文件描述符被挂断,
//    EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，
//            这是相对于水平触发(Level Triggered)来说的,
//    EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，
//            如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里,
epoll_ctl(epfd, op, fd, event)

// events则是分配好的 epoll_event结构体数组,
// epoll将会把发生的事件复制到 events数组中(events不可以是空指针,
// 内核只负责把数据复制到这个 events数组中, 不会去帮助我们在用户态中分配内存.
epoll_wait(epfd, events[in], max_events, timeout)
```

关于 ET、LT 两种工作模式：

epoll 有两种工作模式：LT（水平触发）模式和 ET（边缘触发）模式。

默认情况下，epoll 采用 LT 模式工作，这时可以处理阻塞和非阻塞套接字，而上表中的 EPOLLET 表示可以将一个事件改为 ET 模式。 ET 模式的效率要比 LT 模式高，它只支持非阻塞套接字。

ET 模式与 LT 模式的区别在于：

当一个新的事件到来时，ET 模式下当然可以从 epoll_wait 调用中获取到这个事件，可是如果这次没有把这个事件对应的套接字缓冲区处理完，在这个套接字没有新的事件再次到来时，在 ET 模式下是无法再次从 epoll_wait 调用中获取这个事件的；而 LT 模式则相反，只要一个事件对应的套接字缓冲区还有数据，就总能从 epoll_wait 中获取这个事件。因此，在 LT 模式下开发基于 epoll 的应用要简单一些，不太容易出错，而在 ET 模式下事件发生时，如果没有彻底地将缓冲区数据处理完，则会导致缓冲区中的用户请求得不到响应。默认情况下，Nginx 是通过 ET 模式使用 epoll 的。

```cpp
// 规定：fd[0] → r； fd[1] → w
pipe(pipfd[2]) // 成功：0；失败：-1，设置 errno
```

## IO Manager

### IOManager private 字段

```cpp
int m_epfd = 0;
int m_tickleFds[2];
std::atomic<size_t> m_pendingEventCount = {0};
RWmutexType m_mutex;
std::vector<FdContext *> m_fdContexts;
```

- `m_epfd`: 在 `IOManager::IOManager(...)` 时候调用 `epoll_create(size)` 创建句柄，
  在`IOManager::~IOManager()` 时候 关闭句柄.

- `m_tickleFds[2]`: 在 `IOManager::tickle()`, 在 scheduler 有空闲线程的情况下
  (通过原子计数`m_idleThreadCount`), 向写端写入一个值， 在 `IOManager::idle()` 中的循环逻辑中的针对 `epoll_wait` 返回事件组中， 标识 fd 是 tickleFds[0] 读端相关事件进行取出操作。

- `m_pendingEventCount`: 对等待的事件的原子计数， 执行`IOManager::addEvent` 时候计数会增加， 执行`IOManger::delEvent/cancelEvent/cancelAll`, 以及`IOManager::idle` 计数会减少.

- `m_fdContexts`: 当前 IO Manager 保存的 socket 句柄，1.5 扩容。

用 pipe 读端事件(EPOLLIN | EPOLLET) 替代使用信号量来唤醒 IOManager 线程池中的 空闲线程。

### IOManager Constructor and Destructor

```cpp
IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
  // 初始化 epoll 句柄 和 pipe 句柄
  m_epfd = epoll_create(5000);
  pipe(m_tickleFds);


  // 设置 epoll 事件， 监听fd读和设置为（ET边缘触发模式）,  对象是 m_tickleFds[0] 读端
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = m_tickleFds[0];

  // 修改 m_tickleFds[0] 属性
  // `F_SETFL` - 设置给arg描述符状态标志
  //  非阻塞I/O，如果read调用没有可读取的数据，或者如果write操作将阻塞，
  //    则read或write调用将返回-1和EAGAIN错误
  fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);

  // 给epdf 添加 针对m_tickleFds[0]读端 监听读和写事件
  epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);

  // 初始化 socket 句柄
  contextResize(32);

  // 调度器启动
  start();
}

IOManager::~IOManager() {
  // 调度器停止
  stop();

  // 关闭 epoll 句柄 和 管道读写端
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  // 释放 socket 上下文
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (m_fdContexts[i]) {
      delete m_fdContexts[i];
    }
  }
}
```

### IOManager 实现的虚函数

```cpp
void tickle() override;
bool stopping() override;
void idle() override;
```

#### IOManager::tickle()

通知 IOManager 有任务到来。

```cpp
// 如果，目前没有空闲的线程可以接受任务
if (!hasIdleThreads()) {
  return;
}
// 从写端写入一个值, 通知空闲线程结束idle.
write(m_tickleFds[1], "T", 1);
```

#### IOManager::stopping()

同时 `m_pendingEventCount == 0`, 即没有等待任务时候才可以进入退出状态。

#### IOManager::idle()

```cpp
// epoll_event 数组， 存放 epoll_wait 返回的响应事件组
epoll_event *events = new epoll_event[MAX_EVENTS]();
// shared_ptr 指针默认的释放规则是不支持释放数组的，
// 只能自定义对应的释放规则，才能正确地释放申请的堆内存。
// 1. 管理数组的所有权。shared_events 持有数组元素的一份共享所有权，
// 即使在代码执行过程中，shared_events 可以在堆上共享指向相同数组的其他智能指针。
// 在所有持有该共享所有权的对象都释放他们的引用之后，数组内存将被正确地释放，避免内存泄漏。
//
// 2. 定义在释放共享所有权时要执行的自定义释放规则。
// 在这个例子中，当 shared_events 离开它的作用域时，
// 它的第二个参数所指定的 lambda 函数将被调用，用于删除通过 new[] 分配的数组内存。
std::shared_ptr<epoll_event> shared_events(
    events,
    [](epoll_event *ptr) { delete[] ptr; } // 针对 events 的释放
);

while (true) {
  if (stopping()) {
    // 退出
    break;
  }

  // 如果 epoll_wait 返回响应的事件组，代表 事件组 的实际大小
  int rt = 0;
  // 通过 epoll_wait 获取响应 事件组
  do {
    static const int MAX_TIMEOUT = 5000;
    // epoll_wait will blocking until:
    // 1. a file descriptor delivers an event;
    // 2. teh call is interrupted by a single handler;
    // 3. the timeout expires(MAX_TIMEOUT milliseconds);
    rt = epoll_wait(m_epfd, events, MAX_EVENTS, MAX_TIMEOUT);

    // on success, return the amount of file descriptors which are in ready
    // state, or zero timeout expires. on failure, return -1 and set the
    // errno indicate the error.

    if (rt < 0 && errno == EINTR) {
      // failure and interrupt by signal, retry epoll_wait
    } else {
      // timeout expired or having the amount of `rt` events
      break;
    }
  } while (true);

  // rt < 0 有错误
  // rt == 0 超时 (无响应事件)
  // rt > 0 进行响应事件处理

  // 处理 rt 个事件
  for (int i = 0; i < rt; ++i) {
    // coming events collection
    epoll_event &event = events[i];

    // having value in pipe, (sent in IOManager::tickle), take out from read
    // end of pipe
    // 事件优先
    if (event.data.fd == m_tickleFds[0]) {
      uint8_t dummy;
      // 保证取出,
      while (read(m_tickleFds[0], &dummy, 1) == 1)
        ;
      continue;
    }

    // 恢复 fd 上下文 (fd_ctx), epoll_ctl 时候传入的data.
    FdContext *fd_ctx = (FdContext *)event.data.ptr;

    FdContext::MutexType::Lock lock(fd_ctx->mutex);

    // 判断响应事件时候报错 / 对方关闭连接
    // 1. the socket happen error
    // 2. the socket has been closed
    if (event.events & (EPOLLERR | EPOLLHUP)) {
      // add event interested (read data and write data)
      //  check with the old event collection
      event.events |= (EPOLLIN | EPOLLOUT) & (uint32_t)fd_ctx->events;
    }

    Event real_events = Event::NONE;
    // 可读
    if (events->events & EPOLLIN) {
      real_events |= Event::READ;
    }
    // 可写
    if (events->events & EPOLLOUT) {
      real_events |= Event::WRITE;
    }

    // 对当前 事件组 时候注册了
    if ((fd_ctx->events & real_events) == Event::NONE) {
      continue;
    }

    // 剩下要处理事件
    Event rest_events = (fd_ctx->events & ~real_events);

    // 如果还有事件， 添加修改
    // 没有就是 删除  监听
    int op =
        ((Event)rest_events != Event::NONE) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    // et 模式
    event.events = (uint32_t)(EPOLLET | rest_events);

    int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
    if (rt2) { continue; }

    // 触发 读事件
    if (!!(real_events & Event::READ)) {
      fd_ctx->triggerEvent(Event::READ);
      --m_pendingEventCount;
    }
    // 触发 写事件
    if (!!(real_events & Event::WRITE)) {
      fd_ctx->triggerEvent(Event::WRITE);
      --m_pendingEventCount;
    }
  }

  // 完成 事件处理 / 没有事件处理， 切到调度器安排fiber
  // swap out current fiber
  Fiber::ptr cur = Fiber::GetThis();
  auto raw_ptr = cur.get();
  cur.reset();

  raw_ptr->swapOut();
}
```
