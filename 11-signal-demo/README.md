[TOC]

## 前言

[信号](https://zh.wikipedia.org/zh-hans/Unix%E4%BF%A1%E5%8F%B7)是进程间通信的一种方式。信号这个概念对应的C接口，在Linux上好使的很，但在windows上聊胜于无。

本文尝试：

* Linux上信号的使用。

* 聊胜于无的std::signal，UCRT中的signal，boost::asio::signal_set

* windows上信号的替代方案 -- (同步)事件

* 没有尝试：libuv中的信号，windows-kill
  
---  

## Linux上的信号

详细见《unix环境高级编程》第十章 信号。本文不涉及多线程中信号的处理，我也不清楚。详细见《unix环境高级编程》12.8 线程和信号。

**信号是软件中断**。信号提供了一种处理异步事件的方法。[signal(7) - Linux manual page](https://man7.org/linux/man-pages/man7/signal.7.html) 可以查看Linux支持的信号列表。

下面这个示例：接收`SIGTERM`(终止请求信号)和用户用户定义的信号。在Linux下，强烈建议使用`sigaction`, 而非`signal`:  [c - What is the difference between sigaction and signal? - Stack Overflow](https://stackoverflow.com/questions/231912/what-is-the-difference-between-sigaction-and-signal)

```cpp
#include <iostream>
#include <signal.h>

#define SIGNOTHING SIGRTMIN + 1

void signal_handle(int signum) {

  if (signum == SIGTERM) { /*终止请求信号*/
    std::cout << "receive term signal, will exit now." << std::endl;
    exit(0);
  } else if (signum == SIGUSR1) { /*用户定义信号*/
    std::cout << "receive user1 signal." << std::endl;
  } else if (signum == SIGNOTHING) { /*自定义信号*/
    std::cout << "receive nothing signal." << std::endl;
  } else {
    std::cout << "unsolve signal: " << signum << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "pid: " << getpid() << std::endl;
  std::cout << "SIGRTMIN: " << SIGRTMIN << std::endl;
  std::cout << "__SIGRTMIN: " << __SIGRTMIN << std::endl;

  struct sigaction action;
  action.sa_handler = signal_handle;
  action.sa_flags = SA_RESTART; // 被信号中断的系统调用能自动重启
  // 等信号处理函数,来的信号被加入到屏蔽字中,等待信号处理函数返回后再恢复
  sigfillset(&action.sa_mask);

  // 修改信号的默认动作-通常的默认动作是SIG_IGN,SIG_DFL
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGUSR1, &action, nullptr);
  sigaction(SIGNOTHING, &action, nullptr);

  while (1) {
    sleep(1);
  }
}
```

下面简单介绍下上面的代码过程。

程序对于接收到的信号，有默认的动作，如忽略或者终止。当要更改信号动作时，我们需要先创建`struct sigaction`的`action`。`action.sa_handler`指向信号处理函数。`sa_mask`是一个信号集，当调用该信号处理函数时，`sa_mask`中的信号都要加到信号屏蔽字中。待信号处理函数执行完，再将信号屏蔽字回复到之前。这样的好处是，当前信号处理完毕前，阻塞到来的信号（不能同时进入信号处理函数）。若同一种信号多次发生， 通常并不将它们加  入队列， 所以如果在某种信号被阻塞时， 它发生了5次， 那么对这种信号解除阻塞后， 其信号处理函数通常只会被调用一次。

接着使用`sigaction`函数，修改指定信号的默认动作。

除了`SIGUSR1`、`SIGUSR2`这两个用户自定义的信号外，用户还可以使用`SIGRTMIN`\~`SIGRTMAX`之间的信号。

```shell
$ kill -l
 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP
 6) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL     10) SIGUSR1
11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM
16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP
21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR
31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1  36) SIGRTMIN+2  37) SIGRTMIN+3
38) SIGRTMIN+4  39) SIGRTMIN+5  40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8
43) SIGRTMIN+9  44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13 52) SIGRTMAX-12
53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9  56) SIGRTMAX-8  57) SIGRTMAX-7
58) SIGRTMAX-6  59) SIGRTMAX-5  60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2
63) SIGRTMAX-1  64) SIGRTMAX
```

需要注意的是，`SIGRTMIN` 和 `__SIGRTMIN` 不同。从个人感觉上来说，不要使用下划线开头的变量。相关见：[linux kernel - Why is the integer value of SIGRTMIN (first real-time signal) 34 and not 32? - Unix &amp; Linux Stack Exchange](https://unix.stackexchange.com/questions/573075/why-is-the-integer-value-of-sigrtmin-first-real-time-signal-34-and-not-32)

有一点稍微不好的是，`SIGRTMIN`宏是一个函数，不能用在switch-case后面。

下面使用[pkill(1) - Linux man page](https://linux.die.net/man/1/pkill)，给我们的测试程序发送信号，进行测试，输出如下。

```shell
./linux-signal 
pid: 526
SIGRTMIN: 34
__SIGRTMIN: 32
receive user1 signal. # pkill -SIGUSR1 linux-signal
receive nothing signal. # pkill -SIGRTMIN+1 linux-signal
receive term signal, will exit now. # pkill -SIGTERM linux-signal
```

---

## 跨平台的信号API

windows系统对信号支持的不好。所以，跨平台的信号API也没啥大的作用。

[ucrt中的signal](https://learn.microsoft.com/zh-cn/cpp/c-runtime-library/reference/signal?view=msvc-170)中可以指定的信号只有6个。而其中的两个，`SIGILL`和 `SIGTERM` 信号不会在 Windows 下生成, 包括它们是为了实现 ANSI 兼容性。 而`raise`只能给进程自身发送信号, 无法进程与进程之间发送信号。[taskkill](https://learn.microsoft.com/zh-cn/windows-server/administration/windows-commands/taskkill), 用于结束进程或任务，它并不能发送信号。所以，**在windows下，信号不是一种进程间通信的方法**(因为windows无法进程与进程之间发送信号)。

如果某天，我们需要在windows下使用`signal`(是的,我在一个开源代码中看到这个使用了, 它同时运行在windows和Linux上), 那不如直接使用[std::signal](https://zh.cppreference.com/w/cpp/utility/program/signal)。

下面是个使用示例，进程给自己发送信号。

```cpp
#include <csignal>
#include <iostream>

namespace {
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal) { gSignalStatus = signal; }

int main() {
  // 安装信号处理函数
  std::signal(SIGTERM, signal_handler);

  std::cout << "信号值：" << gSignalStatus << '\n';
  std::cout << "发送信号：" << SIGTERM << '\n';
  std::raise(SIGTERM);
  std::cout << "信号值：" << gSignalStatus << '\n';
}
```

而, 如果项目正在使用boost, [boost::asio::signal_set](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio/reference/signal_set.html), 也一个不错的选择。下面是个简单的示例代码。

```cpp
#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  boost::asio::io_context io_context;
  boost::asio::signal_set signals(io_context);
  signals.add(SIGINT);
  signals.add(SIGTERM);
  signals.async_wait([](boost::system::error_code ec, int signo) {
    std::cout << "receive signo " << signo;
  });
  io_context.run();
  return 0;
}
```

---

## windows上的信号替代方案

那windows下有信号的替代方案吗。有的, [windows C++ 如何向另一个进程发送信号](https://segmentfault.com/q/1010000007829156)。

> 对方有窗口可用SendMessage或者PostMessage。无窗口可以用内核同步变量Event，接收方用WaitForSingleObject来等待，发送方用SetEvent置信。

我不想写win32的窗口程序。因为过程又臭又长, 见：[创建传统的Windows桌面应用程序](https://da1234cao.blog.csdn.net/article/details/114818786)。

下面是无窗口的信号替代方案 --  [同步) (事件对象](https://learn.microsoft.com/zh-cn/windows/win32/sync/event-objects)。

替代方案好吗？不好。因为信号是种软中断。而, windows中的事件需要在一个循环中, 使用`WaitForMultipleObjects`不断等待一个或多个指定对象的信号。

代码比较简单。一个是事件的接收端，`WaitForMultipleObjects`等待信号。一个是事件的发送端。事件对象在` SetEvent`设置为发出信号。相关的函数自行查阅windows文档。下面是示例。

公共头文件。

```cpp
#pragma once

/**
 * 一般是不建议在头文件中定义变量的,(没有static)会导致变量重复定义。
 * 这里加上static,每个引入的文件,会创建自己的signal_name,导致创建多个
 * 这样写比较省事,虽然语法编译上可以通过,但是和实际想表达的全局变量不同
 * https://blog.csdn.net/weibo1230123/article/details/83000786
 */
#define EVENT_SIZE 2
static const char *events_name[] = {"_test_event_one_", "_test_event_two_"};

```

等待对象信号的接收端。

```cpp
#include "common.h"
#include <Windows.h>
#include <iostream>

int main(int argc, char *argv[]) {
  HANDLE events[EVENT_SIZE] = {0};
  for (int i = 0; i < EVENT_SIZE; i++) {
    events[i] = CreateEvent(NULL, TRUE, FALSE, events_name[i]);
  }

  while (1) {
    DWORD event_index =
        WaitForMultipleObjects(EVENT_SIZE, events, FALSE, INFINITE);
    if (event_index == WAIT_OBJECT_0) {
      std::cout << "receive _test_event_one_" << std::endl;
      ResetEvent(events[event_index]);
    } else if (event_index == WAIT_OBJECT_0 + 1) {
      std::cout << "receive _test_event_two_" << std::endl;
      ResetEvent(events[event_index]);
    } else {
      std::cout << "WaitForMultipleObjects return " << event_index << std::endl;
      break;
    }
  }

end:
  // 进程终止时，系统会自动关闭句柄。 事件对象在关闭其最后一个句柄时被销毁
  for (int i = 0; i < EVENT_SIZE; i++) {
    CloseHandle(events[i]);
  }
}
```

发送对象信号的发送端。

```cpp
#include "common.h"
#include <Windows.h>
#include <iostream>

void test_two() {
  HANDLE event_two_handle = CreateEvent(NULL, TRUE, FALSE, events_name[1]);
  SetEvent(event_two_handle);
}

int main(int argc, char *argv[]) {
  HANDLE event_one_handle = CreateEvent(NULL, TRUE, FALSE, events_name[0]);
  SetEvent(event_one_handle);
  if (!ResetEvent(event_one_handle)) {
    std::cout << "event_one_handle reset: " << GetLastError();
  }
  // receiver可能还没来得及重新reset
  if (!SetEvent(event_one_handle)) {
    std::cout << "event_one_handle set: " << GetLastError();
  }

  HANDLE event_two_handle = CreateEvent(NULL, TRUE, FALSE, events_name[1]);
  SetEvent(event_two_handle);
  Sleep(1000);
  ResetEvent(event_two_handle);
  SetEvent(event_two_handle);
  return 0;
}
```

输出如下：

```shell
receive _test_event_one_
receive _test_event_two_
receive _test_event_two_
```

其中需要注意的是: 同一个事件对象，重复调用[createEvent](https://learn.microsoft.com/zh-cn/windows/win32/api/synchapi/nf-synchapi-createeventa), 后一次调用会重置前一次的设置吗？ 答案是不会。当事件对象已经存在，第二次调用`createEvent`, 应该只是相当于open。相关链接见：[Will using CreateEvent to create/open an even that already exists reset the signal?](https://stackoverflow.com/questions/3732015/will-using-createevent-to-create-open-an-even-that-already-exists-reset-the-sign)

---

## 其他

还有些信号相关的API或者仓库，我并没有细看。

* [libuv - Signal handle](http://docs.libuv.org/en/v1.x/signal.html)

* [windows-kill](https://github.com/ElyDotDev/windows-kill)
