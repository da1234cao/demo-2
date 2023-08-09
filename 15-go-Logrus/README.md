[toc]

# 前言

日志可以用于排查问题。在C++中，我尝试过：[boost log简介](https://blog.csdn.net/sinat_38816924/article/details/125117096)、[spdlog日志库的封装使用](https://blog.csdn.net/sinat_38816924/article/details/126192561)。但我还是比较喜欢[plog](https://github.com/SergiusTheBest/plog)，因为它简单。

Go 标准库提供了一个日志库log。它的使用可见：[Go 每日一库之 log](https://darjun.github.io/2020/02/07/godailylib/log/)。但是，它有个致命的缺点，没有日志等级。它可以很好的用于日常写demo，但是不适合稍微大点的代码。

参考[在Github中stars数最多的Go日志库集合](https://studygolang.com/articles/11995)，本文尝试[logrus](https://github.com/Sirupsen/logrus)。

---

# Logrus入门

本节内容翻译/摘录自：[Sirupsen/logrus](https://github.com/Sirupsen/logrus)、[Go 每日一库之 logrus](https://darjun.github.io/2020/02/07/godailylib/logrus/)

Logrus 是 Go (golang)的结构化日志记录器，完全兼容标准库日志记录器的 API。

Logrus 进入维护模式。我们不会引入新的特性。因为它已经被很多项目依赖。引入新特性，可能会破坏他们的项目，这是不希望发生的事情。这并不意味着 Logrus 死了。Logrus 将继续保持安全性、(向后兼容的) bug 修复和性能(在这些方面我们受到接口的限制)。我认为，Logrus 最大的贡献在于，在 Golang 结构化日志的广泛应用中发挥了一定作用。似乎没有理由在 Logrus V2中进行重大的、突破性的迭代，因为出色的 Go 社区已经独立地构建了这些迭代。社区中也有很多优秀的日志库，如Zerolog、 Zap 和 Apex。

---

## 设置日志等级

设置日志等级未debug。

```go
package main

import (
	"github.com/sirupsen/logrus"
)

func main() {
	logrus.SetLevel(logrus.DebugLevel)

	logrus.Trace("trace msg")
	logrus.Debug("debug msg")
	logrus.Info("info msg")
	logrus.Warn("warn msg")
	logrus.Error("error msg")
	logrus.Fatal("fatal msg")
	logrus.Panic("panic msg")
}
```

默认输出到标准输出。输入内容如下。

```go
DEBU[0000] debug msg                                    
INFO[0000] info msg                                     
WARN[0000] warn msg                                     
ERRO[0000] error msg                                    
FATA[0000] fatal msg                                    
exit status 1
```

---

## 设置输出格式

上面默认使用的时`logrus.SetFormatter(&logrus.TextFormatter{})`，输出在tty中会有颜色。我们也可以设置为`logrus.SetFormatter(&logrus.JSONFormatter{})`格式，将字段记录为 JSON。其他的第三方format见官网。也可以自定义format。

```go
package main

import (
	"github.com/sirupsen/logrus"
)

func main() {
	logrus.SetLevel(logrus.DebugLevel)
	logrus.SetFormatter(&logrus.JSONFormatter{})
	// logrus.SetFormatter(&logrus.TextFormatter{})

	logrus.Trace("trace msg")
	logrus.Debug("debug msg")
	logrus.Error("error msg")
}
```

输出如下。

```shell
{"level":"debug","msg":"debug msg","time":"2023-08-09T18:37:58+08:00"}
{"level":"error","msg":"error msg","time":"2023-08-09T18:37:58+08:00"}
```

---

## 设置输出文件名和函数

```go
package main

import (
	"github.com/sirupsen/logrus"
)

func main() {
	logrus.SetLevel(logrus.DebugLevel)
	logrus.SetFormatter(&logrus.TextFormatter{})
	// logrus.SetFormatter(&logrus.TextFormatter{
	// 	DisableColors: true,
	// 	FullTimestamp: true,
	// })
	logrus.SetReportCaller(true)

	logrus.Trace("trace msg")
	logrus.Debug("debug msg")
	logrus.Error("error msg")
}
```

输入如下。

```shell
DEBU[0000]/xxx/demo-2/15-go-Logrus/demo-1.go:17 main.main() debug msg                                    
ERRO[0000]/xxx/demo-2/15-go-Logrus/demo-1.go:18 main.main() error msg
```

---

## 添加字段

```go
package main

import (
	log "github.com/sirupsen/logrus"
)

func main() {
	log.SetLevel(log.DebugLevel)
	// log.SetFormatter(&log.TextFormatter{})
	// log.SetReportCaller(true)

	log.WithFields(log.Fields{
		"animal": "walrus",
	}).Debug("A walrus appears")

	contextLogger := log.WithFields(log.Fields{
		"common": "this is a common field",
		"other":  "I also should be logged always",
	})

	contextLogger.Info("I'll be logged with common and other field")
	contextLogger.Info("Me too")
}
```

输出如下。

```shell
DEBU[0000] A walrus appears                              animal=walrus
INFO[0000] I'll be logged with common and other field    common="this is a common field" other="I also should be logged always"
INFO[0000] Me too                                        common="this is a common field" other="I also should be logged always"
```

---

## 使用新的logger实例

```go
package main

import (
	"os"

	log "github.com/sirupsen/logrus"
)

// Create a new instance of the logger. You can have any number of instances.
var FileLog = log.New()

func main() {

	file, err := os.OpenFile("logrus.log", os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0666)
	if err == nil {
		FileLog.SetOutput(file)
	} else {
		FileLog.Info("Failed to log to file, using default stderr")
	}
	FileLog.SetFormatter(&log.TextFormatter{})
	FileLog.SetLevel(log.DebugLevel)

	FileLog.Debug("hello world")
}
```

日志输出到文件中。

---

## 其他-略

* hook: 每条日志输出前都会执行钩子的特定方法。
* Logger 作为 io.Writer
* 本身不提供日志Rotation。
* 工具：通过配置文件初始化logger
* Logrus 可以注册一个或多个在记录任何致命级别消息时将调用的函数
* 默认情况下，Logger 受并发写操作的mutex保护。

# 一般实践


我习惯了[plog](https://github.com/SergiusTheBest/plog)的日志输出，包含：时间，等级，进程id，输出日志的函数以及所在行，日志信息。最后，日志文件可以回滚。

使用第三方[nested-logrus-formatter](https://github.com/antonfisher/nested-logrus-formatter)设置格式。使用[lumberjack](https://www.cnblogs.com/jssyjam/p/11845475.html)控制日志回滚。示例如下：

```go
package main

import (
	nested "github.com/antonfisher/nested-logrus-formatter"
	log "github.com/sirupsen/logrus"
	"gopkg.in/natefinch/lumberjack.v2"
)

func init() {
	logger := &lumberjack.Logger{
		Filename: "logrus.log",
		// 单位是 MB
		MaxSize: 500,
		// 最大过期日志保留的个数
		MaxBackups: 3,
		// 保留过期文件的最大时间间隔,单位是天
		MaxAge: 28, //days
		// 是否需要压缩滚动日志, 使用的 gzip 压缩
		Compress: true, // disabled by default
	}
	log.SetOutput(logger)

	log.SetFormatter(&nested.Formatter{
		NoColors:      true,
		HideKeys:      true,
		ShowFullLevel: true,
	})
	log.SetReportCaller(true)
	log.SetLevel(log.DebugLevel)
}

func main() {
	log.Debug("hello world")
}
```

输出到文件中，如下：

```shell
Aug  9 18:32:44.025 [DEBUG] hello world (/xxx/demo-2/15-go-Logrus/demo-1.go:33 main.main)
```