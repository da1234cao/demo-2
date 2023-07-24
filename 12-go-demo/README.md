[toc]

## 前言

工作上, 目前主要使用C++。使用C++进行网络编程, 是有点痛苦的事情。比如：如何发起一个http/https的请求并解析返回内容; 如何提供一个restful的https服务;

尝试了解下go。看能否让go来替代C++中网络编程的部分功能。

本文目标：
1. 安装go的编程环境
2. 写个比hello world稍微高级一点的demo代码。

---
## 安装

### windows下go的安装

```shell
# 查找go的包
winget.exe search golang
...
Go Programming Language GoLang.Go                   1.20.6   Tag: golang winget
...

# 安装
winget.exe install -e --id GoLang.Go --version 1.20.6

go.exe version
go version go1.20.6 windows/amd64

# 可以看到环境当前用户的PATH环境变量中多了一条
%USERPROFILE%\go\bin

# 系统的环境变量中也多了一条
C:\Program Files\Go\bin

# Golang设置网络代理
# ref:https://zhuanlan.zhihu.com/p/572151744
go env -w GOPROXY=https://goproxy.cn,direct
```

### Linux下go的安装

```shell
sudo apt install golang-go
# Golang设置网络代理
# ref:https://zhuanlan.zhihu.com/p/572151744
go env -w GOPROXY=https://goproxy.cn,direct
```

在编译外部项目,遇到go版本偏低的时候,我们需要下载最新的包：[Download and install](https://go.dev/doc/install)


### 编译器/编辑器插件

**vscode**

参考: [VsCode Go插件配置最佳实践指南](https://zhuanlan.zhihu.com/p/320343679)

* 先安装[Go in Visual Studio Code](https://code.visualstudio.com/docs/languages/go)插件。
* 后面根据提示：启用go language server; 安装go tools。

我目前没有配置调试，相关见：[Go语言实战笔记（二十三）| Go 调试](https://www.flysnow.org/2017/06/07/go-in-action-go-debug)

---

## hello world

在开始下面内容之前，建议先过一遍[Hello, 世界](https://go.dev/tour/welcome/1)，以对go语法有个基本认识。

下面开始写demo。

我们先初始化一个module。[go module](http://c.biancheng.net/view/5712.html)是Go语言默认的依赖管理工具。下面的模块路径为`go-flag`。**模块路径标识模块并充当模块中包导入路径的前缀的路径**。一般写成仓库路径。这里是自己写demo用，随便写一个就好。模块相关的内容更多自行阅读：[Go Modules Reference](https://go.dev/ref/mod#go-mod-init)、[Module 相关的命令](https://learnku.com/docs/go-mod/1.17/mod-commands/11444#a204d0)。

```shell
mkdir go-flag
go mod init go-flag
```

下面是一个go的demo，用以拷贝文件。接下来，我们使用这个demo逐行介绍语法。

```go
package main

import (
	"flag"
	"fmt"

	cp "github.com/otiai10/copy"
)

var (
	srcPath string
	dstPath string
)

func init() {
	flag.StringVar(&srcPath, "src", "", "source path")
	flag.StringVar(&dstPath, "dst", "", "dest path")
}

func main() {
	flag.Parse()
	err := cp.Copy(srcPath, dstPath)
	if err != nil {
		fmt.Println(err)
	}
}
```

* 首先是[Go语言包的基本概念](http://c.biancheng.net/view/5394.html)。Go语言是使用包来组织源代码的。
* Go语言的包借助了目录树的组织形式，一般包的名称就是其源文件所在目录的名称。(我这里只有一个文件,命名为main也没关系)
* 使用 import 关键字导入使用的包。(至于如何导入项目中自行编写的包,本文不涉及)
* 本文导入, [flag](https://pkg.go.dev/flag)用以接收命令行参数; [fmt](https://pkg.go.dev/fmt)用以格式化输出; [copy](https://pkg.go.dev/github.com/otiai10/copy)进行文件拷贝。
* 初始化每个包，会自动执行init函数。把命令行参数的设置放在init函数中，是个不坏的方法。见：[详解 Go 语言中的 init () 函数](https://learnku.com/go/t/47178)

对于写C习惯的程序猿，必然想知道, go中：长选项是否有对应的短选项; 选项能否放在一个结构体中; 然而，flag包很朴素，也很好用。似乎也没有必要支持这两个功能。

运行。

```shell
# 移除未使用的依赖项，更新依赖项的版本
go mod tidy

# 运行程序
go run copy.go --src=xxx --dst=xxx
```