[toc]

# go入门实践二-tcp服务端

## 前言

上一篇，我们通过[go语言的hello-world入门](https://blog.csdn.net/sinat_38816924/article/details/131901629)，搭建了go的编程环境，并对go语法有了简单的了解。本文实现一个go的tcp服务端。借用这个示例，展示：接口、协程、bufio的使用，与简单的go项目管理；

---

## 接口与方法

首先，我们需要了解go中接口的语法。首先阅读[欢迎使用 Go 指南-方法和接口](https://tour.go-zh.org/list), 了解基本使用。然后到[Go 语言接口-菜鸟教程](https://www.runoob.com/go/go-interfaces.html)中看一个简单的示例。最后有个[理解 Go interface 的 5 个关键点](https://sanyuesha.com/2017/07/22/how-to-understand-go-interface/) (吐槽：示例中的变量命名太烂。类似的也可以阅读[Go Interfaces 使用教程](https://learnku.com/go/t/38843)), 来个小结。

好了，有了上面的基础后，我们来考虑编程。

假定这里需要实现一个tcp的服务端。拓展考虑下：实现一个udp服务端；实现一个http服务端；实现一个socks5服务端。

从C++的角度来考虑：创建一个server类，其中包含一些虚函数。需要tcp服务器，则创建一个继承server类的tcp子类；需要udp服务器，则创建一个继承server类的udp子类；需要http服务端，则创建继承tcp类的http子类；需要socks5服务器，则创建继承tcp和udp类的socks5子类；

事情到了go中，则有所不同。go中没有类与继承。go不是根据类型可以容纳的数据类型来设计抽象，而是根据类型可以执行的操作来设计抽象。interface 是一种具有一组方法的类型。如果一个类型实现了一个 interface 中所有方法，我们说类型实现了该 interface。

代码如下。修改自：[go入门指南-15.1. tcp 服务器](https://learnku.com/docs/the-way-to-go/151-tcp-server/3703)、[Go语言实现TCP服务端和客户端](https://cloud.tencent.com/developer/article/1733034)

```go
package server

// tips1: 包中需要导出的内容,需要大写字母开头

import (
	"bufio"
	"fmt"
	"net"
	"strconv"
)

type Server interface {
	start()
}

type TcpServer struct {
	Ip   string
	Port int
}

func (server *TcpServer) Start() {
	fmt.Println("tcp server start...")

	// 创建 listener
	listener, err := net.Listen("tcp", server.Ip+":"+strconv.Itoa(server.Port))
	if err != nil {
		fmt.Println("Error listening:", err.Error())
		return
	}

	// 监听并接受来自客户端的连接
	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting", err.Error())
			return
		}
		go tcpConnProcess(conn)
	}
}

func tcpConnProcess(conn net.Conn) {
	defer conn.Close()
	// reader := bufio.NewReader(conn)
	for {
		reader := bufio.NewReader(conn) // 错误-应该写在外层
		var buf [128]byte
		n, err := reader.Read(buf[:])
		if err != nil {
			fmt.Println("Error read", err)
			break
		}
		recvStr := string(buf[:n])
		fmt.Println("receive:", recvStr)
		conn.Write([]byte(recvStr))
	}
}
```

---

## 并发-协程

上面代码中对于协程的使用倒是很简单，使用协程来处理每个连接。下面，我们阅读些关于协程的链接。

首先了解基本的goroutine 和 channel的基本概念：[欢迎使用 Go 指南-并发](https://tour.go-zh.org/list)、[Go by Example 中文版: 协程](https://gobyexample-cn.github.io/goroutines)

看过上面任意一个链接，我们可以了解到下面内容。协程(goroutine) 是轻量级的执行线程。通道(channels) 是连接多个协程的管道。你可以从一个协程将值发送到通道，然后在另一个协程中接收。默认情况下，通道是 无缓冲 的，这意味着只有对应的接收（<- chan） 通道准备好接收时，才允许进行发送（chan <-）。 有缓冲通道 允许在没有对应接收者的情况下，缓存一定数量的值。我们可以使用通道来同步协程之间的执行状态。Go 的 选择器（select） 让你可以同时等待多个通道操作。 将协程、通道和选择器结合，是 Go 的一个强大特性...

上面是协程和通道的基本概念，更多的可以阅读下面链接：

[Go语言协程使用最佳实践](https://zhuanlan.zhihu.com/p/374464199): 处理协程崩溃；除了使用channel进行协程控制，sync.WaitGroup也可用来协程的通过(sync本身和协程关系不大?)。

[Go编程时光-4.9 学习 Go 协程：巧妙利用 Context](https://golang.iswbm.com/c04/c04_09.html)：协程的退出。

看到协程的使用，直觉上来说，它可能是一个用户线程。进程是不同线程之间共享资源的集合；线程是内核调度的单位。假设一个线程的时间片没有用完，但是又被一个读取调用阻塞。如果此时这个内核线程中有多个用户线程，则可以在用户层进行切换，继续占用CPU进行运行。详细见：[linux进程](https://da1234cao.blog.csdn.net/article/details/117852542)

---

## 项目管理-包的导入

本文的示例代码，使用module进行管理。我在这个上面踩了些坑。项目管理的介绍，可以翻看下：[第三章：项目管理](https://golang.iswbm.com/chapters/p03.html)。以本文的示例为例，简单顺下流程。

* `go mod init go-tcp-demo`：初始化一个Modules。这时候，可以看到出现一个go.mod文件。它包含模块的引用路径，最低的go版本要求，依赖包。**`go-tcp-demo`并充当模块中包导入路径的前缀的路径**。
* 一个模块中包含多个包(package)，但通常只有一个main package。main package中的main函数，是程序入口。
* 但是，一个mod下也可以有多个main package，需要处于不同目录。
* import导入的是一个路径，会导入该路径下所有的包。
* **通常包名是所在目录的名称**。
* 变量和函数使用驼峰命名法。**如果变量或者函数需要被包外引用，变量的首字母需要大写**。
* 如果需要同时修改多个存在依赖关系的mod，可以参考：[Go 1.18工作区模式最佳实践](https://juejin.cn/post/7084584958307598344)

---

## bufio包使用

上面代码中，给每个连接套一个用户层的缓冲区。因为，从io读取数据，可能是耗时/耗资源的操作。使用io用户层的缓冲区，可以提高效率。

但使用不当，可能也会带来一些问题。比如，上面代码中，当缓冲区存在超过128字节的时候，更多的内容被舍弃。

bufio本身的源码相对比较简单，可以阅读下。更多见：[golang-bufio](https://pkg.go.dev/bufio)、[Go语言使用buffer读取文件](http://c.biancheng.net/view/4595.html)

> buffer 是缓冲器的意思，Go语言要实现缓冲读取需要使用到 bufio 包。bufio 包本身包装了 io.Reader 和 io.Writer 对象，同时创建了另外的 Reader 和 Writer 对象，因此对于文本 I/O 来说，bufio 包提供了一定的便利性。

---

## 其他代码

这里是调用package server，创建一个tcp server。

```go
package main

import (
	"go-tcp-demo/server"
)

func main() {
	s := server.TcpServer{Ip: "127.0.0.1", Port: 10000}
	s.Start()
}
```

这里是客户端的代码。

```go
package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
)

func main() {
	conn, err := net.Dial("tcp", "127.0.0.1:10000")
	if err != nil {
		fmt.Println("Error dial", err)
		return
	}
	defer conn.Close()

	inputReader := bufio.NewReader(os.Stdin)
	for {
		input, isPrefix, err := inputReader.ReadLine()
		if isPrefix || err != nil {
			fmt.Println("The entered single line content is too long or there is an error", err)
			continue
		}
		_, err = conn.Write(input)
		if err != nil {
			fmt.Println("Error in conn write", err)
			continue
		}

		buf := [512]byte{}
		n, rerr := conn.Read(buf[:])
		if rerr != nil {
			fmt.Println("Error in conn read", err)
			continue
		}
		fmt.Println(string(buf[:n]))
	}
}
```


