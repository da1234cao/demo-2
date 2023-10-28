[toc]

## 前言

C++的标准库中，没有包含网络库。我需要(熟悉)一个网络库，将其作为日常网络编程中的“瑞士军刀”。

这里，我选择学习使用[boost_asio](https://www.boost.org/doc/libs/1_83_0/doc/html/boost_asio.html)。

我选择这个库有两个原因：其一，我选择库的顺序是，C++标准库 > boost库> 特定功能的知名的C++库 == QT库。C++标准库中没有的功能优先到boost中去找。其二是，socket编程在windows和linux上是有差异的。为了保证代码在平台之间的可移植性，socket并非一个好的选择。

对于asio库，我目前只能“学习使用”，而不能“学习”。因为，目前我缺少模板编程buff的加成。后面需要补上。

---

## 定时器

下面内容来自：[boost-asio-Tutorial-Basic Skills](https://www.boost.org/doc/libs/1_83_0/doc/html/boost_asio/tutorial.html)

本节，我们了解asio中定时器的使用。(定时器最基本的功能是,到指定时间后,执行相应的动作。注意,这是一个异步过程。异步的原理有些难，我不会。这里有个相关视频，感兴趣可以看看：[CppCon 2016: Michael Caisse “Asynchronous IO with Boost.Asio"](https://www.youtube.com/watch?v=rwOv_tw2eA4))

---

### 同步使用定时器

创建一个定时器，并等待它过期。代码如下。

```c++
#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  boost::asio::io_context io;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));
  t.wait();
  std::cout << "hello world" << std::endl;
  return 0;
}
```

1. 所有的asio类都可以使用 "asio.hpp" 头文件引入。
2. 所有使用asio的程序，需要至少有一个I/O执行上下文，例如io_context或thread_pool对象。提供对I/O的访问能力。
3. 接下来，我们声明一个boost::asio::steady_timer类型的对象。它将io_context的引用作为第一个参数。构造函数的第二个参数将计时器设置为从现在起5秒过期。
4. steady_timer::wait()函数，等待定时器过期。

---

### 异步使用定时器

下面程序演示，定时器的异步使用。定时器过期后，调用指定函数(完成处理程序)。

```c++
#include <boost/asio.hpp>
#include <iostream>

void print(const boost::system::error_code & /*e*/) {
  std::cout << "Hello, world!" << std::endl;
}

int main(int argc, char *argv[]) {
  boost::asio::io_context io;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));
  t.async_wait(&print);
  io.run();
  return 0;
}
```

1. 我们调用steady_Timer::async_wait()函数来执行异步等待。异步等待结束时调用print 的函数。
2. 最后，我们必须在io_context对象上调用io_context::run()成员函数。**asio库保证只从当前调用io_context::run()的线程调用完成处理程序**。因此，除非调用io_context::run()函数，否则将永远不会调用异步等待完成的完成处理程序。
3. 在调用io_context::run()之前，请记住给io_context一些工作。例如，如果我们省略了上面对steady_timer::async_wait()的调用，io_context将没有任何工作要做，因此io_context::run()将立即返回。

---

### 将参数绑定到完成处理程序

在上面的基础上，当我们想给完成处理程序传递参数，但是处理程序又没有额外参数位置。很自然，我们需要使用bind。

```c++
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>

void print(const boost::system::error_code & /*e*/,
           boost::asio::steady_timer *t, int *count) {
  if (*count < 5) {
    std::cout << *count << std::endl;
    ++(*count);
    t->expires_at(t->expiry() + boost::asio::chrono::seconds(1));
    t->async_wait(
        boost::bind(print, boost::asio::placeholders::error, t, count));
  }
}

int main() {
  boost::asio::io_context io;
  int count = 0;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(1));
  t.async_wait(
      boost::bind(print, boost::asio::placeholders::error, &t, &count));
  io.run();
  std::cout << "Final count is " << count << std::endl;
  return 0;
}
```

上面是官方的示例代码，print函数中使用的是boost::asio::steady_timer的指针。C++中不要使用裸指针，可以用智能指针封装下。

那，为什么不能使用引用作为参数呢？代码修改为引用后会报错。报错如下：`use of deleted function boost::asio::basic_waitable_timer`。

```c++
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>

void print(const boost::system::error_code & /*e*/,
           boost::asio::steady_timer &t, int &count) {
  if (count < 5) {
    std::cout << count++ << std::endl;
    t.expires_at(t.expiry() + boost::asio::chrono::seconds(1));
    t.async_wait(
        boost::bind(print, boost::asio::placeholders::error, t, count));
    // t.async_wait(boost::bind(print, boost::asio::placeholders::error,
    //                          boost::ref(t), count));
  }
}

int main(int argc, char *argv[]) {
  int count = 0;
  boost::asio::io_context io;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(1));
  t.async_wait(boost::bind(print, boost::asio::placeholders::error, t, count));
  // t.async_wait(boost::bind(print, boost::asio::placeholders::error,
  //                          boost::ref(t), count));
  io.run();
  return 0;
}
```

有点奇怪。steady_timer的赋值构造和拷贝构造被禁止了，并且这两个函数是私有的。但这不影响引用的使用，

```c++
private:
// Disallow copying and assignment.
  basic_waitable_timer(const basic_waitable_timer&) BOOST_ASIO_DELETED;
  basic_waitable_timer& operator=(
      const basic_waitable_timer&) BOOST_ASIO_DELETED;
```

吃完晚饭后，我重新看这段代码。想到了`Boost.Ref`。对于boost::bind，它的参数是通过传值拷贝的，就算被绑定的函数声明为引用，也不会影响到实参，这时对于引用的参数，我们需要使用boost::ref。问题就此解决。

---

### 在多线程程序中同步完成处理程序

正如我们所知，asio库提供了一种保证，即完成处理程序将仅从当前正在调用io_context::run()的线程中调用。在不同线程中调用io_context::run()，可以并发执行完成处理程序。当处理程序可能访问共享的、线程不安全的资源时，我们需要一种同步方法。

strand类模板是一个执行器适配器，它保证对于通过它调度的处理程序，一个正在执行的处理程序将被允许在下一个开始之前完成。无论调用io_context::run() 的线程数量如何，都可以保证这一点。通过将处理程序绑定到同一个链，我们可以确保它们不能并发执行。

```c++
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

class printer {
public:
  printer(boost::asio::io_context &io)
      : strand_(boost::asio::make_strand(io)),
        timer1_(io, boost::asio::chrono::seconds(1)),
        timer2_(io, boost::asio::chrono::seconds(2)), count_(0) {
    timer1_.async_wait(boost::asio::bind_executor(
        strand_, boost::bind(&printer::print1, this)));

    timer2_.async_wait(boost::asio::bind_executor(
        strand_, boost::bind(&printer::print2, this)));
  }

  ~printer() { std::cout << "Final count is " << count_ << std::endl; }

private:
  void print1() {
    if (count_ < 10) {
      std::cout << "Timer 1: " << count_ << std::endl;
      ++count_;
      timer1_.expires_at(timer1_.expiry() + boost::asio::chrono::seconds(1));
      timer1_.async_wait(boost::asio::bind_executor(
          strand_, boost::bind(&printer::print1, this)));
    }
  }

  void print2() {
    if (count_ < 10) {
      std::cout << "Timer 2: " << count_ << std::endl;
      ++count_;
      timer2_.expires_at(timer2_.expiry() + boost::asio::chrono::seconds(1));
      timer2_.async_wait(boost::asio::bind_executor(
          strand_, boost::bind(&printer::print2, this)));
    }
  }

private:
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::asio::steady_timer timer1_;
  boost::asio::steady_timer timer2_;
  int count_;
};

int main() {
  boost::asio::io_context io;
  printer p(io);
  boost::thread t(boost::bind(&boost::asio::io_context::run, &io));
  io.run();
  t.join();

  return 0;
}
```

因为编译器不会将对象的成员函数隐式转换成函数指针。所以bind函数参数中，必须在成员方法前添加&。

编译的时候，需要链接下boost_thread库。输出如下：

```c++
Timer 1: 0
Timer 1: 1
Timer 2: 2
Timer 1: 3
Timer 2: 4
Timer 1: 5
Timer 2: 6
Timer 1: 7
Timer 2: 8
Timer 1: 9
Final count is 10
```

