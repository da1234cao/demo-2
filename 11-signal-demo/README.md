[TOC]

## 前言

[信号](https://zh.wikipedia.org/zh-hans/Unix%E4%BF%A1%E5%8F%B7)是进程间通信的一种方式。信号这个概念对应的C接口，在Linux上好使的很，但在windows上聊胜于无。

本文尝试：

* Linux上信号的使用。

* 聊胜于无的std::signal，UCRT中的signal，boost::asio::signal_set

* windows上信号的替代方案 -- (同步)事件

* 没有尝试：libuv中的信号，windows-kill
  
  

## Linux上的信号

详细见《unix环境高级编程》第十章 信号。本文不涉及多线程中信号的处理，我也不清楚。详细见《unix环境高级编程》12.8 线程和信号。

**信号是软件中断**。信号提供了一种处理异步事件的方法。

[signal(7) - Linux manual page](https://man7.org/linux/man-pages/man7/signal.7.html) 可以查看Linux支持的信号列表。

下面这个示例，接收`SIGTERM`(终止请求信号)和用户用户定义的信号。在Linux下，强烈建议使用`sigaction`, 而非`signal`:  [c - What is the difference between sigaction and signal? - Stack Overflow](https://stackoverflow.com/questions/231912/what-is-the-difference-between-sigaction-and-signal)

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

程序对于接收到的信号，有默认的动作，如忽略或者终止。当要更改信号动作时，我们需要先创建`struct sigaction`的`action`。`action.sa_handler`指向信号处理函数。`sa_mask`是一个信号集，当调用该信号处理函数时，`sa_mask`中的信号都要加到信号屏蔽字中。待信号处理函数执行完，再将信号屏蔽字回复到之前。这样的好处时，当当前信号处理完毕前，阻塞到来的信号（不能同能同时进入信号处理函数）。若同一种信号多次发生， 通常并不将它们加  
入队列， 所以如果在某种信号被阻塞时， 它发生了5次， 那么对这种信号解除阻塞后， 其信号处理函数通常只会被调用一次。

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



## 跨平台的信号API

## windows上的信号替代方案

## 其他

```shell
vcpkg.exe install libuv:x64-windows
```
