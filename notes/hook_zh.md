# Hook

## Brief introduction

from gpt:

- C++ 中的函数钩子（hook）是一种编程技术，用于替代或在函数执行前后注入额外功能或逻辑。

- 在 C++ 中，可以通过函数指针或虚函数来实现函数钩子。具体来说，在函数钩子中，将原始函数指针存储在一个变量中，并定义一个与其函数签名相似的新函数。新函数中的代码可以包含在原始函数之前或之后执行的一个或多个其他功能。然后，使用替代函数指针来调用新函数。

- C++ 中使用函数钩子最常用的场景是在运行时注入代码，用于对函数的输入、输出或内部变量进行监视或更改。例如，可以在图像处理函数中使用钩子来记录输入图像的大小或输出图像的处理时间等信息。注意，在使用钩子时，需要格外小心，因为它可以对程序的运行时表现造成复杂的影响或安全风险。

- 此外，C++ 中还有一些其他的技术可以实现类似的功能或目的，如函数代理、装饰者等。这些技术的选择取决于特定的应用程序需求和性能要求。

本框架中需要 hook 来对系统相关 阻塞型 api 作异步等待处理。

hook 相关的函数签名:

```cpp
// -- sleep hook --
typedef unsigned int (*sleep_fun)(unsigned int seconds);
typedef int (*usleep_fun)(useconds_t usec);
typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);

// -- socket hook --
typedef int (*socket_fun)(int domain, int type, int protocol);
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr,
                           socklen_t addrlen);
typedef int (*accept_fun)(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen);

// -- socket read hook --
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);

// -- socket write hook --
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len,
                              int flags, const struct sockaddr *dest_addr,
                              socklen_t addrlen);
typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);

// -- socket close --
typedef int (*close_fun)(int fd);

// -- fcntl --
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);

// -- ioctl --
typedef int (*ioctl_fun)(int fd, unsigned long request, ...);

// -- socket get/set options --
typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval,
                              socklen_t *optlen);
typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);
```

基本原理操作：

1. [extern "C"](https://www.cnblogs.com/skynet/archive/2010/07/10/1774964.html) 仅仅表示遵循 C 连接编译规则, 在 header 里面定义与 hook 相关 api 的同样格式的函数指针 (name_fun 和 name_f).
2. 定义 hook 原函数: name_fun name_f = nullptr;
3. 在 main 函数之前的初始化操作 定义： name_f = (name_fun)dlsym(RTLD_NEXT, "name")

- name_f： hook 相关 api 的对应函数。
- name_fun: hook 相关 api 的同一规格函数指针。
- dlsym: [reference](https://www.cnblogs.com/anker/p/3746802.html) 通过句柄和连接符名称获取函数名或者变量名, `RTLD_NEXT`: 可以在对函数实现所在动态库名称未知的情况下完成对库函数的替代。[reference](https://blog.csdn.net/caspiansea/article/details/51337727) <- 加载动态库

## Implementation

每个线程有一可本线程变量，用于记录当前线程下使用 hook 的 api 时时候使用 hook 配置。

```cpp
static thread_local bool t_hook_enable = false;
```

设置一个单例 fdManager 来管理所有的 fdContext

上下文信息 FdCtx:

```cpp
// Whether initialization is complete.
bool isInit() const { return m_isInit; }

// Whether is as socket file descriptor.
bool isSocket() const { return m_isSocket; }

// Whether this file descriptor is closed.
bool isClosed() const { return m_isClosed; }

// Set user initiative to set non-blocking.
void setUserNonblock(bool v) { m_userNonblock = v; }
// Get whether user intervention to set non-blocking.
bool getUserNonblock() const { return m_userNonblock; }

// Set System non-blocking.
void setSysNonblock(bool v) { m_sysNonblock = v; }
// Get whether System non-blocking.
bool getSysNonblock() const { return m_sysNonblock; }

// Set file descriptor timeout.
// @param: type - SO_RCVTIMEO(read timeout) / SO_SNDTIMEO(write timeout)
void setTimeout(int type, uint64_t v);
// Get file descriptor timeout.
uint64_t getTimeout(int type);
```

FdCtx::init():

```cpp
m_recvTimeout = -1;
m_sendTimeout = -1;

struct stat fd_stat;
// `fstat` returns information of file
if (-1 == fstat(m_fd, &fd_stat)) {
  m_isInit = false;
  m_isSocket = false;
} else {
  m_isInit = true;
  m_isSocket = S_ISSOCK(fd_stat.st_mode);
}

if (m_isSocket) {
  // 针对socket fd修改flags
  int flags = fcntl_f(m_fd, F_GETFL, 0);
  if (!(flags & O_NONBLOCK)) {
    // flags 本身未设置 非阻塞
    fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
  }
  m_sysNonblock = true;
} else {
  m_sysNonblock = false;
}

m_userNonblock = false;
m_isClosed = false;
return m_isInit;
```

FdManager:

```cpp
// Acquire/create a file descriptor context.
// @params: auto_create - if the file descriptor is not existed in m_datas,
// create one and add it to the collection.
FdCtx::ptr get(int fd, bool auto_create = false);

// Remove a file descriptor.
void del(int fd);
```

### sleep, usleep, nanosleep hook

基本相似实现只是定时的精度不同

```cpp
// 判断是否启动hook
if (!apexstorm::t_hook_enable) {
  return sleep_f(seconds);
}

// 获取当前线程以及IOManger， 用于添加定时器任务以及指定
// 计时结束后在被epoll_wait返回后把该fiber重新加入到 调度器的调度队列中.
apexstorm::Fiber::ptr fiber = apexstorm::Fiber::GetThis();
apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();

// 写法不同
// iom->addTimer(seconds * 1000,
//               std::bind([iom, fiber]() { iom->schedule(fiber); }));

// 回调函数转化
// void apexstorm::Scheduler::schedule(cb, int) ->
// (void(apexstorm::Scheduler::*)(apexstorm::Fiber::ptr, int thread))

iom->addTimer(seconds * 1000,
  std::bind(
    ( // 函数指针
      void(apexstorm::Scheduler::*)(apexstorm::Fiber::ptr, int thread)
    )&
      // 对应的有参函数
          apexstorm::IOManager::schedule,
      // 绑定的对象
          iom,
      // 对应的有参函数的参数
          fiber, -1
  )
);

// 让出当前fiber, 等计时结束再被调度归来
apexstorm::Fiber::YieldToHold();
return 0;
```

### socket hook

```cpp
// 判断是否启动hook
if (!apexstorm::t_hook_enable) {
  return socket_f(domain, type, protocol);
}
// 调用原socket创建 socket 句柄
int fd = socket_f(domain, type, protocol);

// 在 FdManager 注册句柄
apexstorm::FdMgr::GetInstance()->get(fd, true);
```

### close hook

```cpp
// check hook enable
apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
if (ctx) {
  auto iom = apexstorm::IOManager::GetThis();
  // 取消 所有 fd 相关事件
  if (iom) {
    iom->cancelAll(fd);
  }
  // 注销 fd
  apexstorm::FdMgr::GetInstance()->del(fd);
}
return close_f(fd);
```

### accept, socket read / write hook

template `do_io` 复用大部分 hook api:
完美转发： [std::forward](https://www.jianshu.com/p/b90d1091a4ff)

```cpp
// OriginFun  hook的原始api
// Args       可变传参
template <typename OriginFun, typename... Args>
static ssize_t do_io(
            int fd,                     // 文件句柄
            OriginFun fun,              //  hook 原始的 api
            const char *hook_fun_name,  // hook 原始的 api 名称
            uint32_t event,             // IOManager 事件
            int timeout_so,             // 超时类型
            Args &&...args              // 转发参数
) {
  if (!apexstorm::t_hook_enable) {
    // if not enabling hooking, call original function and parsing parameter
    // pack
    return fun(fd, std::forward<Args>(args)...);
  }

  // acquire the file descriptor corresponding context
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    // this fd may not register in FdManager, maybe created by non-hooking way.
    return fun(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClosed()) {
    // Bad file number
    errno = EBADF;
    return -1;
  }

  //! may cause error (judgement condition)
  if (!ctx->isSocket() || !ctx->getUserNonblock()) {
    // no socket or user dont't wanted to non-blocking
    return fun(fd, std::forward<Args>(args)...);
  }

  // acquire corrsponding (read/write) timeout from fd context
  uint64_t timeout = ctx->getTimeout(timeout_so);
  // create a delaying resource
  std::shared_ptr<timer_info> tinfo(new timer_info);

// goto label, in order to get data
retry:
  // n indicates the amount bytes of data already read from or written into.
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  // handler errno code, Interrupted system call
  while (n == -1 && errno == EINTR) {
    // until the interrupted end up.
    n = fun(fd, std::forward<Args>(args)...);
  }
  // handler errno code, Try again
  // 1. the io buffer has not data when reading, or io buffer is full when
  // writing
  // 2. the resource was locked.
  // 3. fcntl / ioctl operation cannot be completed immediately or for a short
  // period of time.
  if (n == -1 && errno == EAGAIN) {
    apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
    apexstorm::Timer::ptr timer;
    // condition variable weak reference
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout != (uint64_t)-1) {
      // having timeout value

      // add condition delaying timer
      timer = iom->addConditionTimer(
          timeout,
          [winfo, fd, iom, event]() {
            // condition variable is not existed
            auto t = winfo.lock();
            // or being cancelled
            if (!t || t->cancelled) {
              return;
            }
            // set Connection timed out
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (apexstorm::IOManager::Event)(event));
          },
          winfo);
    }

    // add relevant listening event about this fd.
    int rt = iom->addEvent(fd, (apexstorm::IOManager::Event)(event));
    if (rt) {
      // adding failure
      APEXSTORM_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      // adding success, yield out current fiber
      APEXSTORM_LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
      apexstorm::Fiber::YieldToHold();
      APEXSTORM_LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
      // yield come back condition:
      // 1.timer expired
      // 2.data comes
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      // [mentioned]: data initialized?
      goto retry;
    }
  }
  return n;
}
```

然后例如实现 socket api read:

```cpp
ssize_t read(int fd, void *buf, size_t count) {
  return apexstorm::do_io(fd, read_f, "read",
                          (uint32_t)apexstorm::IOManager::Event::READ,
                          SO_RCVTIMEO, buf, count);
}
```

### connect hook

对 connect 添加一个 timeout 计时器， 超时值从 config 中取出

具体实现与`do_io`类似, 只是注册了相关事件以及作定时等待, 然后对连接后的 socket 作 getsockopt 检测.

当一个 socket 发生错误的时候，将使用一个名为 `SO_ERROR` 的变量记录对应的错误代码，这又叫做 pending error，`SO_ERROR` 为 0 时表示没有错误发生, `*optval` 指向的区域将存储错误代码，而 `SO_ERROR` 被设置为 0。

### fcntl hook

对 fcntl 的 F_SETFL 和 F_GETFL 做 hook 操作。

```cpp
case F_SETFL: {
  int arg = va_arg(va, int);
  va_end(va);
  // params: int
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
    // this fd context doesn't register to the fd manager
    // or this fd is closed
    // or this fd is not a socket fd
    return fcntl_f(fd, cmd, arg);
  }
  // set user non-blocking flags to this socket fd
  ctx->setUserNonblock(arg & O_NONBLOCK);
  // check whether is system wanted to non-blocking
  if (ctx->getSysNonblock()) {
    arg |= O_NONBLOCK;
  } else {
    arg &= ~O_NONBLOCK;
  }
  return fcntl_f(fd, cmd, arg);
} break;
case F_GETFL: {
  va_end(va);
  // param: empty
  int arg = fcntl_f(fd, cmd);
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
    // this fd context doesn't register to the fd manager
    // or this fd is closed
    // or this fd is not a socket fd
    return arg;
  }
  // whether user wanted to non-blocking
  if (ctx->getUserNonblock()) {
    return arg | O_NONBLOCK;
  } else {
    return arg & ~O_NONBLOCK;
  }
} break;
```

### ioctl hook

ioctl 是设备驱动程序中对设备的 I/O 通道进行管理的函数.
[reference](https://blog.csdn.net/gemmem/article/details/7268533)

```cpp
va_list va;
va_start(va, request);
void *arg = va_arg(va, void *);
va_end(va);
// param: void*

// requesting non-blocking
if (FIONBIO == request) {
  // a integer pointer
  bool user_nonblock = !!*(int *)arg;
  // non-blocking = 1
  apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
    // this fd context doesn't register to the fd manager
    // or this fd is closed
    // or this fd is not a socket fd
    return ioctl_f(fd, request, arg);
  }
  ctx->setUserNonblock(user_nonblock);
}
return ioctl_f(fd, request, arg);
```

### getsockopt, setsockopt hook

getsockopt 暂时不需要 hook
setsockopt hook part:

```cpp
if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      apexstorm::FdCtx::ptr ctx = apexstorm::FdMgr::GetInstance()->get(sockfd);
      if (ctx) {
        // convert optval -> timeout structure
        const timeval *v = (const timeval *)optval;
        ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
      }
    }
  }
```

## example

测试情景： 在一个线程上开启协程调度， 并且执行两个 http 访问和一个定时器任务

```cpp
// 定时器任务
void test_sleep() {
  apexstorm::IOManager *iom = apexstorm::IOManager::GetThis();
  // 睡眠 2 s
  iom->schedule([]() {
    sleep(2);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 2";
  });

  // 睡眠 3 s
  iom->schedule([]() {
    sleep(3);
    APEXSTORM_LOG_INFO(g_logger) << "sleep 3";
  });
  APEXSTORM_LOG_INFO(g_logger) << "test_sleep";
}

// socket 任务
void socket_operation() {
  // 当前线程已经开启 hook

  // 如阻塞网络io配置

  // socket fd, tcp 字符流
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  // socket setting
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  // ipv4
  addr.sin_family = AF_INET;
  // addr.sin_port = htons(8080);
  // inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  // 目标 port
  addr.sin_port = htons(80);
  // 目标 address
  inet_pton(AF_INET, "110.242.68.66", &addr.sin_addr.s_addr);

  APEXSTORM_LOG_INFO(g_logger) << "begin connect";
  // socket connect
  int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));
  APEXSTORM_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

  if (rt) {
    return;
  }

  // 往 socket 写入数据
  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  rt = send(sock, data, sizeof(data), 0);
  APEXSTORM_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
  if (rt <= 0) {
    return;
  }

  std::string buff;
  buff.resize(4096);

  // 从 socket 读出数据
  rt = recv(sock, &buff[0], buff.size(), 0);
  APEXSTORM_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

  if (rt <= 0) {
    return;
  }

  buff.resize(rt);
  APEXSTORM_LOG_INFO(g_logger) << buff;
}

int main() {
  // 启动单线程的多协程IO调度器
  apexstorm::IOManager iom;
  // 加入3个调度任务, 理论编写顺序与执行顺序无关
  iom.schedule(test_sleep);
  iom.schedule(socket_operation);
  iom.schedule(socket_operation);
}
```

运行结果：

```log
fiber    log     logger           code line               log message
number
0       [DEBUG] [system]        lib/fiber.cpp:57        Fiber::Fiber
0       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=1
0       [INFO]  [system]        lib/scheduler.cpp:107   0x7fffc8edc510 stopped
1       [INFO]  [system]        lib/scheduler.cpp:166   run
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=2
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=3
3       [INFO]  [root]  tests/test_hook.cpp:24  test_sleep      [<开始睡眠任务>]
3       [INFO]  [root]  tests/test_hook.cpp:37  begin connect   [<开始socket connect>]
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=4
4       [INFO]  [root]  tests/test_hook.cpp:37  begin connect   [<开始是socket connect>]
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=5
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=6
2       [INFO]  [system]        lib/iomanager.cpp:486   Idle
3       [INFO]  [root]  tests/test_hook.cpp:39  connect rt=0 errno=115
3       [INFO]  [root]  tests/test_hook.cpp:47  send rt=19 errno=115
3       [INFO]  [system]        lib/hook.cpp:191        do_io<recv>
4       [INFO]  [root]  tests/test_hook.cpp:39  connect rt=0 errno=11
4       [INFO]  [root]  tests/test_hook.cpp:47  send rt=19 errno=11
4       [INFO]  [system]        lib/hook.cpp:191        do_io<recv>
3       [INFO]  [system]        lib/hook.cpp:193        do_io<recv>
3       [INFO]  [root]  tests/test_hook.cpp:56  recv rt=300 errno=11
3       [INFO]  [root]  tests/test_hook.cpp:63  HTTP/1.1 200 OK [<其中一个socket返回数据>]
Date: Wed, 14 Jun 2023 03:36:27 GMT
Server: Apache
Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT
ETag: "51-47cf7e6ee8400"
Accept-Ranges: bytes
Content-Length: 81
Cache-Control: max-age=86400
Expires: Thu, 15 Jun 2023 03:36:27 GMT
Connection: Close
Content-Type: text/html


1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=3
4       [INFO]  [system]        lib/hook.cpp:193        do_io<recv>
4       [INFO]  [root]  tests/test_hook.cpp:56  recv rt=381 errno=11
4       [INFO]  [root]  tests/test_hook.cpp:63  HTTP/1.1 200 OK [<其中一个socket返回数据>]
Date: Wed, 14 Jun 2023 03:36:27 GMT
Server: Apache
Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT
ETag: "51-47cf7e6ee8400"
Accept-Ranges: bytes
Content-Length: 81
Cache-Control: max-age=86400
Expires: Thu, 15 Jun 2023 03:36:27 GMT
Connection: Close
Content-Type: text/html

<html>
<meta http-equiv="refresh" content="0;url=http://www.baidu.com/">
</html>

1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=4
1       [DEBUG] [system]        lib/fiber.cpp:89        Fiber::Fiber id=7
5       [INFO]  [root]  tests/test_hook.cpp:17  sleep 2       [<sleep 2 秒 结束>]
1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=5
6       [INFO]  [root]  tests/test_hook.cpp:22  sleep 3       [<sleep 3 秒 结束>]
1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=6
2       [INFO]  [system]        lib/iomanager.cpp:497   name= idle stopping exit
1       [INFO]  [system]        lib/scheduler.cpp:278   idle fiber term
1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=7
1       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=2
0       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=1
0       [DEBUG] [system]        lib/fiber.cpp:117       Fiber::~Fiber id=0
```
