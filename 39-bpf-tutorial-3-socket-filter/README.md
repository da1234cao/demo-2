
[toc]

# ebpf教程(3):使用cmake构建ebpf项目

## 前言

前置阅读要求：

- [什么是 eBPF ? An Introduction and Deep Dive into the eBPF Technology](https://ebpf.io/zh-hans/what-is-ebpf/)
- [libbpf Overview — libbpf documentation](https://libbpf.readthedocs.io/en/latest/libbpf_overview.html)
- [Building BPF applications with libbpf-bootstrap](https://nakryiko.com/posts/libbpf-bootstrap/)

本文示例代码见仓库(代码摘录自libbpf-bootstrap)。

---

## 推荐使用BPF skeleton

ebpf编程分为这几步：编写一个ebpf程序；编译成ebpf字节码；打开/加载/附加/卸载eBPF程序。

上面这个过程，在日常开发中，有两种实现。

我先说我不推荐的方式 -- 使用 `bpf_prog_load()`函数加载ebpf程序。

0. 假设现在有两个文件，app.bpf.c和app.c。app.bpf.c是ebpf程序。app.c是用来加载ebpf程序的用户层程序。
1. 编译app.bpf.c。`clang -O2 -g -target bpf -c app.bpf.c app.bpf.o`。
2. 在app.c中，使用 `bpf_prog_load()`函数加载 `app.bpf.o`。
3. 编译app.c，生成app。
3. 发布程序的时候，发布app和app.bpf.o这两个二进制文件。

我再说我推荐的方式 -- 使用BPF skeleton。

0. 假设现在有两个文件，app.bpf.c和app.c。app.bpf.c是ebpf程序。app.c是用来加载ebpf程序的用户层程序。
1. 编译app.bpf.c。`clang -O2 -g -target bpf -c app.bpf.c app.bpf.o`。
2. 生成skeleton header(app.skel.h)。 `bpftool gen skeleton app.bpf.o > app.skel.h`。
3. 在app.c中，包含 app.skel.h即可。这个头文件中包含ebpf的字节码。
4. 编译app.c，生成app。
5. 发布程序的时候，发布app这一个二进制文件。

此外 BPF skeleton，在使用API的方面，也更加方便。

BPF skeleton的好处是：
* skeleton代码包括 BPF obj文件的字节码表示，简化了分发 BPF 代码的过程。嵌入 BPF 字节码后，无需与应用程序二进制文件一起部署额外的文件。
* BPF skeleton是 libbpf API 的替代接口，用于处理 BPF 对象。skeleton代码抽象出通用的 libbpf API，从而显著简化了从用户空间操作 BPF 程序的代码。
* BPF skeleton为用户空间程序提供了一个使用 BPF 全局变量的接口。skeleton代码将全局变量作为结构体映射到用户空间。结构体接口允许用户空间程序在 BPF 加载阶段之前初始化 BPF 程序，并在之后从用户空间获取和更新数据。
* BPF skeleton提供对所有 BPF map和 BPF program的直接访问，作为结构字段。这消除了使用bpf_object_find_map_by_name()和 bpf_object_find_program_by_name() 进行基于字符串的查找的需要，从而减少了由于 BPF 源代码和用户空间代码不同步而导致的错误。
* 目标文件的嵌入字节码表示确保skeleton和 BPF 目标文件始终同步。

因为有这些好处。所以 libbpf 的文档中写了，‘Using the skeleton code is the recommended way to work with bpf programs.’。

所以，推荐使用BPF skeleton 在BPF编程中。

---

## 使用cmake构建ebpf项目

当我看了 `libbpf-bootstrap` 中的example后，我完全会选择使用 BPF skeleton 。

但是，该如何集成到日常的项目中呢。即，如何使用cmake构建ebpf程序。

`libbpf-bootstrap` 是使用MakeFile构建的。所以，必然也可以使用CMake重写这个过程。但是，手动将MakeFile转换成CMake很是有点麻烦。

但是现在也不用我们自己写了。因为，几周前，仓库中添加了cmake的构建文件：https://github.com/libbpf/libbpf-bootstrap/tree/master/tools/cmake

我尝试了下，很好用。

---

## 最后

libbpf版本的选择。

libbpf API 有着不小的变化。现在已经发布了v1.4.5版本。但是日常开发/部署的机器上还是v0.5版本 -- 2021年发布的版本，已经太老了。

当然可以在cmake中拉取libbpf，然后静态链接。这有点麻烦。


