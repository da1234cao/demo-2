
[toc]

## 前言

日志是程序的重要组成部分。本文介绍rsyslog在C编程中的简单使用。

它的官方文档我没有去读，所以复杂的使用我不知道。

---

## rsyslog的简单使用

### 日志在命令行的使用

在开始在C语言的代码中使用rsyslog之前，我们先看下，如何在命令行中生产日志。这对于shell脚本很有用。

```shell
journalctl -f # 实时查看系统日志

# 命令行的简单使用
logger "hello rsyslog" # journalctl 输出：Jan 08 21:43:53 da1234cao dacao[5056]: hello rsyslog

# 指定日志的tag为"logger"。日志的priorit由两部分组成，一部分是Facility(见后文)，一部分是Severity(日志等级)。
logger -t "logger" -p local0.info "hello local0" # journalctl 输出：Jan 08 21:44:05 da1234cao logger[5141]: hello local0
```

---

### 日志在C语言中使用

#### syslog的API

下面我们看下，如何在C语言中记录日志。主要是下面这三个函数。

```c
#include <syslog.h>
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);
```

`openlog`函数为程序打开到rsyslog的连接。第一个参数是tag，它会出现在每条消息中。通常设置为程序名。为Null的时候，默认设置为程序名。option是下面这些值得组合。

```c
#define	LOG_PID		0x01	/* log the pid with each message */ /*记录每条消息都要包含进程ID。*/
#define	LOG_CONS	0x02	/* log on the console if errors in sending */ /*若日志消息不能通过UNIX域数据报送至syslogd，则将该消息写至控制台*/
#define	LOG_ODELAY	0x04	/* delay open until first syslog() (default) */ /*在第一条消息被记录之前延迟打开至syslogd守护进程的连接*/
#define	LOG_NDELAY	0x08	/* don't delay open */ /*立即打开至syslogd守护进程的UNIX域数据报套接字，不要等到第一条消息已经被记录时再打开。通常，在记录第一条消息之前，不打开该套接字*/
#define	LOG_NOWAIT	0x10	/* don't wait for console forks: DEPRECATED */ /* 不要等待在将消息记入日志过程中可能已创建的子进程。因为在syslog调用wait时，应用程序可能已获得了子进程的状态，这种处理阻止了与捕捉SIGCHLD信号的应用程序之间产生的冲突 */
#define	LOG_PERROR	0x20	/* log to stderr as well */ /*除将日志消息发送给syslogd以外，还将它写至标准出错*/
```

facility则有下面这些选项。

```c
/* facility codes */
#define	LOG_KERN	(0<<3)	/* kernel messages */
#define	LOG_USER	(1<<3)	/* random user-level messages */
#define	LOG_MAIL	(2<<3)	/* mail system */
#define	LOG_DAEMON	(3<<3)	/* system daemons */
#define	LOG_AUTH	(4<<3)	/* security/authorization messages */
#define	LOG_SYSLOG	(5<<3)	/* messages generated internally by syslogd */
#define	LOG_LPR		(6<<3)	/* line printer subsystem */
#define	LOG_NEWS	(7<<3)	/* network news subsystem */
#define	LOG_UUCP	(8<<3)	/* UUCP subsystem */
#define	LOG_CRON	(9<<3)	/* clock daemon */
#define	LOG_AUTHPRIV	(10<<3)	/* security/authorization messages (private) */
#define	LOG_FTP		(11<<3)	/* ftp daemon */

	/* other codes through 15 reserved for system use */
#define	LOG_LOCAL0	(16<<3)	/* reserved for local use */
#define	LOG_LOCAL1	(17<<3)	/* reserved for local use */
#define	LOG_LOCAL2	(18<<3)	/* reserved for local use */
#define	LOG_LOCAL3	(19<<3)	/* reserved for local use */
#define	LOG_LOCAL4	(20<<3)	/* reserved for local use */
#define	LOG_LOCAL5	(21<<3)	/* reserved for local use */
#define	LOG_LOCAL6	(22<<3)	/* reserved for local use */
#define	LOG_LOCAL7	(23<<3)	/* reserved for local use */
```

`syslog`函数用来生成一条日志。priority由facility和 level组成。如果没有设置facility，则使用`openlog`中设置的值。如果在调用`syslog`之前没有调用`openlog`,则facility这里被默认设置为 LOG_USER。level有下面选项。

```c
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
```

`closelog`也是可选择的， 因为它只是关闭曾被用于与rsyslogd守护进程进行通信的描述符。

---

#### 最小示例

介绍完上面三个API，我们来实现一个简单的demo。

```c
#include <syslog.h>

int main(int argc, char *argv[]) {
  openlog("demo", LOG_CRON | LOG_PID, LOG_USER);
  syslog(LOG_DEBUG, "just use syslog.use openlog");
  closelog();
  return 0;
}
```

查看输出。

```shell
journalctl -t demo
Hint: You are currently not seeing messages from other users and the system.
      Users in groups 'adm', 'systemd-journal' can see all messages.
      Pass -q to turn off this notice.
Jan 08 22:08:33 da1234cao demo[9957]: just use syslog.use openlog
```

如果没有需要，不需要调用`openlog`和`closelog`函数，使用默认值就好。

如下面这样。

```c
#include <syslog.h>

int main(int argc, char *argv[]) {
  // syslog(LOG_DEBUG| LOG_USER, "just use syslog.");
  syslog(LOG_DEBUG, "just use syslog.");
  return 0;
}
```

输出如下。

```shell
journalctl -t demo
Hint: You are currently not seeing messages from other users and the system.
      Users in groups 'adm', 'systemd-journal' can see all messages.
      Pass -q to turn off this notice.
Jan 08 22:08:33 da1234cao demo[9957]: just use syslog.use openlog
Jan 08 22:12:15 da1234cao demo[10841]: just use syslog.
```

---

#### rsyslog的配置文件

上面这些日志，记录在什么文件中呢。我们查看下`/etc/rsyslog.conf`文件。具体的配置语法，我没去看官方文档，但是不影响能看懂。

我们可以看到配置中有这行规则。

```shell
*.*;auth,authpriv.none          -/var/log/syslog
```

* *.*: 表示匹配所有的设备和所有的优先级。这里的*是通配符，表示任何设备和任何优先级的日志消息。
* ;: 分隔符，用于分隔不同的设备和优先级。
* auth,authpriv.none: 表示将auth（身份验证相关的消息）和authpriv（私有的身份验证消息）的日志消息排除在外，不记录到指定的文件中。在这个规则中，* none表示不记录这些特定类型的消息。
* -: 表示将匹配的日志消息写入到指定的文件，但不将其同时写入到其它地方（比如终端或者其它文件）。
* /var/log/syslog: 是指定的日志文件路径，这里是/var/log/syslog。

因此，这一行规则的意思是：将除了auth和authpriv之外的所有设备和优先级的日志消息写入到/var/log/syslog文件中，而auth和authpriv的消息则不记录。

---

### 进阶使用

想要用好rsyslog，还是得看它的文档。比如：日志写入到指定文件、日志回滚、接收另一台机器发送过来的日志、转发日志等。

等我有时间且需要的时候再看下吧。

## 参考

1. [syslog 协议与 Rsyslog 系统日志软件简介 | EisenHao's Note](https://eisenhao.cn:8443/2021/08/07/syslogProtocolAndRsyslog/)
2. 《unix环境高级编程》13.4 出错记录
3. [syslog(3) — Linux manual page](https://man7.org/linux/man-pages/man3/syslog.3.html)