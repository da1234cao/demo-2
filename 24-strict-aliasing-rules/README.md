# C语言中的Strict Aliasing Rule

## 前言

最近有个代码在gcc 4.8.5上编译失败。编译失败的提示是：

```shell
error: dereferencing type-punned pointer will break strict-aliasing rules [-Werror=strict-aliasing]
```

查了下这个报错，有点复杂。大体是不要使用一个类型的指针，去操作另一种指针指向的空间。比如下面这样：

```c
#include <inttypes.h>
#include <stdio.h>

struct internet {
  __uint16_t ip;
};

__uint8_t address[10];

int main(int argc, char *argv[]) {
  address[0] = 1;
  address[1] = 2;

  struct internet *net = (struct internet *)address;
  __uint16_t ip = net->ip;

  printf("%" PRIu8 "\n", address[0]);
  printf("%" PRIu8 "\n", address[1]);
  printf("%" PRIu16 "\n", ip);
}
```

然而，上面这段代码在不同的gcc 11.4.1版本下编译，没有问题。

关于Strict Aliasing Rule的详细解释见：[What is the Strict Aliasing Rule and Why do we care?](https://gist.github.com/shafik/848ae25ee209f698763cffee272a58f8)、[c when would you not want to use strict aliasing?](https://stackoverflow.com/questions/63796726/c-when-would-you-not-want-to-use-strict-aliasing)

我也没有完全搞懂。下面示例，来自这个链接。

## 没有警告不代表没有问题

下面我们来看下这个示例。在常见的gcc版本下编译，应该都能复现。

```c
#include <iostream>

int foo(float *f, int *i) {
  *i = 1;
  *f = 0.f;

  return *i;
}

int main() {
  int x = 0;
  std::cout << x << std::endl; // Expect 0
  int x_ret = foo(reinterpret_cast<float *>(&x), &x);
  std::cout << x_ret << "\n";  // Expect 0?
  std::cout << x << std::endl; // Expect 0?
}
```

首先，我们编译的时候不要开启优化，输出如下：

```shell
g++ -O0 demo-2.cpp -o demo-2

0
0
0
```

接着，我们编译的时候开启优化，输出如下：

```shell
g++ -O2 demo-2.cpp -o demo-2

0
1
0
```

这就比较脑壳痛了。日常开发编译的是debug版本，它没有优化。发布的时候，编译的是release版本，它有一定的编译优化。然后相同的代码，debug和release版本的运行不同。这个问题可能就很难排查。

为什么会出现这种情况？编译器也没有给出警告？

大概是因为优化的时候，编译器看到要返回的是i，和f又没有什么关系，给返回寄存器里面提前填入了i的值。

## 目前

一般来说，日常编程中，即使不同类型的指针，操作相同的内存，也不会出现上面这种情况。所以正常使用就好，不用特地回避，出问题再解决问题。（为什么不事先回避这个问题呢？因为搞不清，那就先不管。）

如果遇到上面这种问题，或者因为这个问题编译失败，怎么办呢？

* 第一种方法是：使用memcpy进行拷贝，不要直接使用不同类型的指针，操作相同的内存。
* 第二种方法是：在gcc的构建选项中添加`-fno-strict-aliasing`选项。但是这会导致整个构建过程都忽略了这个限制。
* 第三种是，可以尝试下使用`__attribute__((optimize("-fno-strict-aliasing")))`修饰函数，但是这不一定有效。
