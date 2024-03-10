# Linux C 中执行shell命令

## 摘要

本文尝试使用exec,system,popen函数，来执行一个shell命令。(1) 如果只需要执行命令后的返回值，不关心标准输出，错误输出，可以使用system函数。(2) 如果希望拿到返回值，标准输出，可以使用popen。(2) 如果前面两个函数都不能满足要求，那使用exec，虽然这个比较麻烦。

---

## 前言

老实说(To be honest), 在Linux c 中，调用exec/system/popen来执行shell命令，都不太完美。exec的缺点是用起来比较麻烦。system和popen是封装的还不够好。

谈论好/坏之前，需要建立评价标准。或者说，需要实现哪些功能，才算好了。我们参考下[Boost.Process](https://www.boost.org/doc/libs/1_72_0/doc/html/process.html#boost_process.introduction),一个C++进程库有哪些功能。

* 创建一个子进程。
* 为子进程设置输入/输出流(为子进程设置输入流，读取子进程的标准输出/错误输出)(同步和异步)。
* 等待进程结束,获取返回码(同步和异步)。
* 终止子进程。

system函数，只能拿到返回值; popen只能设置标准输入，或者标准输出，没法单独获取到错误输出。而想要使用exec函数，优雅又安全的实现上面功能，不容易，挺不容易。

哎呀，凑活着用就是嘞，像生活一样。

---

## exec函数的使用

参考: [exec(3) - Linux manual page](https://man7.org/linux/man-pages/man3/exec.3.html) , 《unix环境高级编程》8.10 函数exec

这个函数不太好写，我也不咋喜欢用，因为有些麻烦。我们看下面这个示例。

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("exec", LOG_PERROR, 0);

  char *cmd = "ls";
  char *ls_argv[] = {"ls", "-alh", "NO_EXIST_FILE", NULL};

  pid_t pid;
  if ((pid = fork()) < 0) {
    syslog(LOG_ERR, "fork error");
  } else if (pid == 0) { /* specify pathname, specify environment */
    if (execvp(cmd, ls_argv) < 0) {
      syslog(LOG_ERR, "execvp error");
    }
  }

  int status;
  if (waitpid(pid, &status, 0) < 0) {
    syslog(LOG_ERR, "wait error");
  } else {
    if (WIFEXITED(status)) {
      int ret = WEXITSTATUS(status);
      syslog(LOG_INFO, "subprocess return code: %d", ret);
    }
  }

  exit(0);
}
```

这个程序很简单，创建一个子进程并执行shell命令。父进程等待子进程结束。

仔细推敲的话，上面的实现是有问题的。

(1) `fork()` 执行失败的时候，会返回-1。而[waitpid(3) - Linux man page](https://linux.die.net/man/3/waitpid)的第一个参数是-1时，表示等待任意一个子进程，而不是我们目前希望的子进程。如果此时没有自进程，它会立即出错返回。
(2) 上面，我们希望拿到子进程执行后的返回码。但是如果程序不是正常(return,exit方式)结束，比如信号终止或者coredump，是拿不到子进程本身的返回值的。

---

## system函数使用

参考：[system(3) - Linux manual page](https://man7.org/linux/man-pages/man3/system.3.html) , 《unix环境高级编程》8.13 函数system

system函数的行为，像是使用fork创建子进程，然后像下面这个调用exec函数。

```c
execl("/bin/sh", "sh", "-c", command, (char *) NULL);
```

父进程在执行命令的期间，会阻塞SIGCHLD信号，忽略SIGINT和SIGQUIT信号。

我挺喜欢system函数。因为它的接口简单，易上手。下面，我们使用system函数，重写下上面的程序。

```c
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("system", LOG_PERROR, 0);

  char *cmd = "ls -alh NO_EXIST_FILE";

  int status = system(cmd);
  if (status == -1) {
    syslog(LOG_ERR, "create subprocess failed: %s", strerror(errno));
    goto err;
  }

  if (WIFEXITED(status)) {
    int ret = WEXITSTATUS(status);
    syslog(LOG_INFO, "subprocess return code: %d", ret);
  }

err:
  exit(0);
}
```

可惜的是，拿不到标准错误输出。这个错误输出直接输出到命令行了，不方便写入日志。

输出如下。

```shell
ls: cannot access 'NO_EXIST_FILE': No such file or directory
system: subprocess return code: 2
```

---

## popen函数的使用

程序员这个行业，某些时候真是太无聊了，必须得花时间在“茴香豆的茴有哪些写法”上。

我们再看看看popen的使用。

参考: [popen(3) - Linux manual page](https://man7.org/linux/man-pages/man3/popen.3.html) , 《unix环境高级编程》15.3 函数popen和pclose

我们继续使用popen实现上面代码的功能。(红烧鱼，清蒸鱼，糖醋鲤鱼，一🐟多吃)

```c
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("system", LOG_PERROR, 0);

  char *cmd = "ls -alh NO_EXIST_FILE 2>&1";
  //   char *cmd = "ls -alh NO_EXIST_FILE";

  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    syslog(LOG_ERR, "create subprocess failed: %s", strerror(errno));
    exit(0);
  }

  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    printf("%s", line);
  }

  int status = pclose(fp);
  if (status == -1) {
    syslog(LOG_ERR, "pclose failed: %s", strerror(errno));
  } else if (WIFEXITED(status)) {
    int ret = WEXITSTATUS(status);
    syslog(LOG_INFO, "subprocess return code: %d", ret);
  }

  exit(0);
}
```

关于popen接口的使用，自行参考官方文档。下面看两个更有意思的问题。

问题一：能否单独获取标准错误输出，而不是将标准错误输出混在标准输出中。

答案是可以，但是不是一个好主意，得引入select这样的函数，来保证可以同时读取两个流。可以参考: [c popen won't catch stderr - Stack Overflow](https://stackoverflow.com/questions/6900577/c-popen-wont-catch-stderr)

问题二：为什么执行`char *cmd = "ls -alh NO_EXIST_FILE";`这样的命令，也获取到标准错误输出。

这个就有点奇怪了。因为按照文档，popen只能获取到标注输出，而获取不到标准错误输出。但是翻了下[coreutils-ls](https://github.com/wertarbyte/coreutils/blob/f70c7b785b93dd436788d34827b209453157a6f2/src/ls.c#L2827)的源码，ls在这里，内部调用的`error`函数，确实是输出到标准错误输出。通过命令行`ls -alh NO_EXIST_FILE 2>/dev/null`也能验证，确实是标准错误输出。但是神奇的是，popen函数尽然可以拿到这个输出。我也不知道为啥。

有句老话，源码面前，了无秘密。可以通过下面这个的方式，调试popn的源码。但是我目前不想探究这个原因。

```shell
# centos系安装glibc源码
sudo debuginfo-install glibc-devel.x86_64
```