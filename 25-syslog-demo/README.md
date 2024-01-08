
[toc]

## 摘要

本文简介rsyslog在C编程中的简单使用。

---

## rsyslog的简单使用

### 日志在命令行的使用

```shell
journalctl -f # 实时查看系统日志

# 命令行的简单使用
logger "hello rsyslog" # journalctl 输出：Jan 08 13:48:15 da1234cao da1234cao[33757]: hello rsyslog

# 指定日志的tag为"logger"。日志的priorit由两部分组成，一部分是Facility(见后文)，一部分是Severity(日志等级)。
logger -t "logger" -p local0.info "hello local0" # journalctl 输出：Jan 08 13:55:40 da1234cao logger[35330]: hello local0
```

---

### 日志在C语言中使用

```shell
# 输入如下
Jan 08 11:09:40 da1234cao demo1[4758]: just use syslog.
```

---

## 参考

1. [syslog 协议与 Rsyslog 系统日志软件简介 | EisenHao's Note](https://eisenhao.cn:8443/2021/08/07/syslogProtocolAndRsyslog/)
2. 《unix环境高级编程》13.4 出错记录