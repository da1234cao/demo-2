[toc]

## 前言

本文不是一篇介绍ScopeGuard作用的博客。本文是一篇使用ScopeGuard博客。

刚工作那会，我被要求写异常安全的代码。异常安全的代码不太好写，但也是那会搞清了异常是什么。

关于异常的介绍，可以阅读下面参考内容：

* 《C++ primer》18.1 异常的处理
* 《Effective C++》Item 29:争取异常安全(exception-safe)的代码

而，使用ScopeGuard是写异常安全代码的好方法，可以参考下面链接：

* [What is ScopeGuard in C++? - Stack Overflow](https://stackoverflow.com/questions/31365013/what-is-scopeguard-in-c)
* [c++ - Does ScopeGuard use really lead to better code? - Stack Overflow](https://stackoverflow.com/questions/48647/does-scopeguard-use-really-lead-to-better-code)

下面介绍，实现ScopeGuard的一些库函数。

---
## Boost.ScopeExit

参考自：

* [Chapter 1. Boost.ScopeExit 1.1.0 - 1.81.0](https://www.boost.org/doc/libs/1_81_0/libs/scope_exit/doc/html/index.html)
* [Chapter 3. Boost.ScopeExit](https://theboostcpplibraries.com/boost.scopeexit)

```cpp
#include <boost/scope_exit.hpp>
#include <iostream>

int *foo() {
  int *i = new int{10};
  BOOST_SCOPE_EXIT(&i) {
    delete i;
    i = 0;
  }
  BOOST_SCOPE_EXIT_END
  std::cout << *i << '\n';
  return i;
}

int main() {
  int *j = foo();
  std::cout << j << '\n';
}
```

Boost.ScopeExit 提供了宏 `BOOST_SCOPE_EXIT`。
* 它接受以逗号分隔的捕获变量列表。如果捕获以符号 `&` 开始，则对捕获变量的引用将在 `Boost.ScopeExit` 主体内可用；否则，将在Boost.ScopeExit 声明之后创建变量的副本，并且副本只在主体内可用（在这种情况下，捕获的变量的类型必须是 `CopyConstructible` ）
	* 变量的捕获和lambda表达式类似。lambda表达式的介绍，详见《C++ primer》10.3.2 节。
	* 在 C++11 编译器上，也可以捕获作用域中的所有变量，需使用特殊宏 `BOOST_SCOPE_EXIT_ALL` 。该捕获列表必须以 `&` 或 `=` 开头，以分别通过引用或值捕获范围内的所有变量。特定变量的额外捕获可以跟在前导的 `&` 或 `=` 之后，它们将覆盖默认的引用或值捕获。
* 最后，Boost.ScopeExit 主体的结尾必须由 `BOOST_SCOPE_EXIT_END` 宏标记。
* Boost.ScopeExit 主体的执行将被延迟到当前作用域的末尾。

请注意，变量 i 在 `BOOST_SCOPE_EXIT` 定义的块末尾设置为 0。 i 然后由 `foo()` 返回并写入 `main()` 中的标准输出流。但是，该示例不显示 0。j 被设置为一个随机值——即在内存被释放之前 `int` 变量所在的地址。 `BOOST_SCOPE_EXIT` 后面的块获得了对 i 的引用并释放了内存。但是由于该块是在 `foo()` 的末尾执行的，因此将 0 分配给 i 为时已晚。 `foo()` 的返回值是在 i 被设置为 0 之前创建的 i 的副本。

输出如下：

```shell
10
0000023EC6216080
```

如果您使用 C++11 开发环境，也可以在 lambda 函数的帮助下使用[RAII](https://zh.cppreference.com/w/cpp/language/raii)来解决资源释放问题（建议使用库函数，不建议自行实现）。

```cpp
#include <iostream>
#include <utility>

template <typename T> struct scope_exit {
  scope_exit(T &&t) : t_{std::move(t)} {}
  ~scope_exit() { t_(); }
  T t_;
};

template <typename T> scope_exit<T> make_scope_exit(T &&t) {
  return scope_exit<T>{std::move(t)};
}

int *foo() {
  int *i = new int{10};
  auto cleanup = make_scope_exit([&i]() mutable {
    delete i;
    i = 0;
  });
  std::cout << *i << '\n';
  return i;
}

int main() {
  int *j = foo();
  std::cout << j << '\n';
}
```

如果明白函数模板推断与移动语义(见：[c++中move和forward详解\_大1234草的博客-CSDN博客](https://blog.csdn.net/sinat_38816924/article/details/129962473))，上面代码很容易看懂，和之前使用BOOST_SCOPE_EXIT是相同的作用。

上面代码不严谨。设想，如果调用者对一个`scope_exit`的对象进行拷贝，那可能导致资源释放两次，从而报错。所以`scope_exit`应该禁止拷贝构造函数和等号的运算符重载（关于这两者的区别见：[c++ - What's the difference between assignment operator and copy constructor? - Stack Overflow](https://stackoverflow.com/questions/11706040/whats-the-difference-between-assignment-operator-and-copy-constructor)）。

再来看一个例子。当 `BOOST_SCOPE_EXIT` 用于在一个范围内定义多个块时，这些块将以相反的顺序执行。如果没有变量将传递给 `BOOST_SCOPE_EXIT` ，则需要指定 `void` 。括号不能为空。如果在成员函数中使用 `BOOST_SCOPE_EXIT` ，并且需要传递指向当前对象的指针，则必须使用this_，而不是 `this` 。

```cpp
#include <boost/scope_exit.hpp>
#include <iostream>

struct x {
  int i;

  void foo() {
    i = 10;
    BOOST_SCOPE_EXIT(void) { std::cout << "last\n"; }
    BOOST_SCOPE_EXIT_END
    BOOST_SCOPE_EXIT(this_) {
      this_->i = 20;
      std::cout << "first\n";
    }
    BOOST_SCOPE_EXIT_END
  }
};

int main() {
  x obj;
  obj.foo();
  std::cout << obj.i << '\n';
}
```

输出如下：

```shell
first
last
20
```

---

## loki-lib.ScopeGuard

参考自：
* [loki-lib/ScopeGuard.h at master · snaewe/loki-lib · GitHub](https://github.com/snaewe/loki-lib/blob/master/include/loki/ScopeGuard.h)
* [永久改变你写异常安全代码的方式（神奇的Loki::ScopeGuard）](https://blog.csdn.net/purewinter/article/details/1860875)

我读了下上面的实现过程，但部分代码还是有点绕，没搞明白。但使用上差不多。

```cpp
#include "loki/ScopeGuard.h"
#include <iostream>

int *foo() {
  int *i = new int{10};
  Loki::ScopeGuard free_i = Loki::MakeGuard([&i]() {
    delete i;
    i = 0;
    std::cout << "free i" << std::endl;
  });

  std::cout << *i << '\n';
  //   free_i.Dismiss();

  return i;
}

int main() {
  int *j = foo();
  std::cout << j << '\n';
}
```

输出如下：

```cpp
10
free i
000001B5C1A20510
```

目前，**并不建议在代码中使用loki-lib的ScopeGuard**。原因应该是，当时创建这个代码的时候，C++标准中还没有引入变长模板参数。代码里面，相同的逻辑，由于参数个数不同，重复多遍。不优雅。（如果不想引入boost库，又想使用ScopeGuard，可以使用下一节中链接里的实现）

---

## 其他

通常，不建议自行造轮子，比如这个ScopeGuard。Boost库中既然有ScopeExit，直接用就好。

自行实现是个麻烦的事情，需要考虑评价标准(即，实现细节)。

还有其他实现，我暂时没有去看源码，比如：

* [scope\_guard | A modern C++ scope guard that is easy to use but hard to misuse.](https://ricab.github.io/scope_guard/)