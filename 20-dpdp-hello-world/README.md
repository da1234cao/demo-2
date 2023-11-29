# DPDK-Hello-World示例应用程序

## 前言

目标: 在linux上安装DPDK的程序编写环境，编写和运行DPDK的hello world程序。

首先，我也不清楚[DPDK](https://www.dpdk.org/about/)具体是个啥。DPDK的目的大概是：原先的网络数据需要从内核层拷贝到用户层，这比较慢。DPDK可以跳过内核，实现更快的数据包处理。

## 环境安装

阅读：[DPDK-系统要求](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html)、[从源代码编译DPDK目标](https://doc.dpdk.org/guides/linux_gsg/build_dpdk.html)

首先是系统要求，需要在内核中启用 HUGETLBFS 选项。我是在wsl-ubuntu中进行程序开发，没找见内核选项。我以前简单编译过内核，知道些内核选项：[内核编译选项简介](https://blog.csdn.net/sinat_38816924/article/details/122025837)。（我在网上找了下，暂时没有看见WSL-ubunut控制内核选项的方法。目前跳过。）

接着是，编译DPDK。emm，目前不清楚那些编译选项的具体内容。那直接使用包管理器安装吧，当前够用就好。

```shell
# almlinux9
# sudo dnf remove dpdk dpdk-devel dpdk-tools 

# ubuntu22
sudo apt install libhugetlbfs-bin dpdk-dev libdpdk-dev
```

## 编写hello-world程序

阅读：[Hello World 示例应用程序](https://doc.dpdk.org/guides/sample_app_ug/hello_world.html)

示例应用程序是可编写的最简单的 DPDK 应用程序的示例。该应用程序只是在每个启用的 lcore 上打印一条“helloworld”消息。

`lcore`是啥？

我们可以看下这个链接: [环境抽象层 (EAL)](https://doc.dpdk.org/guides/prog_guide/env_abstraction_layer.html)

>环境抽象层 (EAL) 负责访问低级资源，例如硬件和内存空间。它提供了一个通用接口，对应用程序和库隐藏了环境细节。初始化例程负责决定如何分配这些资源（即内存空间、设备、定时器、控制台等）。

>术语“lcore”指的是 EAL 线程。DPDK 通常为每个核心固定一个 pthread，以避免任务切换的开销。这可以显着提高性能，但缺乏灵活性并且并不总是高效。当使用多个 pthread 时，EAL pthread 和指定的物理 CPU 之间的绑定不再总是 1:1...

下面是示例代码，修改自：[helloworld](https://github.com/DPDK/dpdk/tree/main/examples/helloworld)

```shell
#include <rte_eal.h>
#include <rte_lcore.h>
#include <stdio.h>

int lcore_hello(__rte_unused void *arg) {
  unsigned int lcore_id = rte_lcore_id();
  printf("hello from core %u\n", lcore_id);
  return 0;
}

int main(int argc, char *argv[]) {
  if (rte_eal_init(argc, argv) < 0) {
    rte_exit(EXIT_FAILURE, "fail in init");
  }
  for (unsigned int lcore_id = rte_get_next_lcore(-1, 1, 0);
       lcore_id < RTE_MAX_LCORE;
       lcore_id = rte_get_next_lcore(lcore_id, 1, 0)) {
    rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
  }

  lcore_hello(NULL);

  rte_eal_mp_wait_lcore(); // Wait until all lcores finish their jobs.

  rte_eal_cleanup();

  return 0;
}
```

