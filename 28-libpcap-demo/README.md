[toc]

# 前言

因为之前简单写过[wireshark入门指北](https://blog.csdn.net/sinat_38816924/article/details/132166085)，所以我知道基本的抓包流程。最近尝试调用[libpcap](https://www.tcpdump.org/manpages/pcap.3pcap.html)的C API接口，顺道整理下。

本文实验如下：
1. 使用[tcpdump](https://www.tcpdump.org/manpages/tcpdump.1.html)命令抓取目标地址为百度的数据包，并将结果保存文件。
2. 使用tcpdump加载上面的数据包。
3. 使用libpcap的API，实现上面的两个过程。

注：本文代码仅考虑在Linux环境下运行。

---

# tcpdump的使用

如果是第一次使用tcpdump，会感觉这个命令的参数咋这么多，令人摸不着头脑。于是，每次需要写tcpdump命令，都要去问chatgpt，就像写正则一样。下面，我们将搞懂这个命令参数的结构，以后写tcpdump命令也会比较轻松。

我们去看它的官方文档，会看到tcpdump命令参数如下：

```shell
tcpdump [ -AbdDefhHIJKlLnNOpqStuUvxX# ] [ -B buffer_size ]
         [ -c count ] [ --count ] [ -C file_size ]
         [ -E spi@ipaddr algo:secret,... ]
         [ -F file ] [ -G rotate_seconds ] [ -i interface ]
         [ --immediate-mode ] [ -j tstamp_type ]
         [ --lengths ] [ -m module ]
         [ -M secret ] [ --number ] [ --print ]
         [ --print-sampling nth ] [ -Q in|out|inout ] [ -r file ]
         [ -s snaplen ] [ -T type ] [ --version ] [ -V file ]
         [ -w file ] [ -W filecount ] [ -y datalinktype ]
         [ -z postrotate-command ] [ -Z user ]
         [ --time-stamp-precision=tstamp_precision ]
         [ --micro ] [ --nano ]
         [ expression ]
```

可以看到参数分为两部分。其中一部分是tcpdump的option。另一部分是expression。expression是一个[pcap-filter](https://www.tcpdump.org/manpages/pcap-filter.7.html)。（这个expression是大名鼎鼎的BPF expression。如果不知道BPF，不影响。我之前接触过点bpf，所以概念上理解起来很轻松：[bpf简介1](https://da1234cao.blog.csdn.net/article/details/115471891)）

这张图，可以很好的显示tcpdump命令的结构。图片来自：[全网最详细的 tcpdump 使用指南](https://www.cnblogs.com/wongbingming/p/13212306.html)。

![tcpdump](tcpdump.png)

1. option：tcpdump命令的选项。
2. expressio：proto、dir和type组成一个BPF完整表达式的基元。这些基元可以通过and,or,not来组合。

下面我们实际使用tcpdump命令试试。

```shell
# 抓包内容输出到标准输出。
## -nn:不把协议和端口号转化成名字; -vv:产生比-v更详细的输出; -i:指定网卡; 引号中的是BPF表达式，表抓取目标地址为百度的数据包
tcpdump -nn -vv -i enp0s3 "dst host www.baidu.com"

# -c:控制抓包个数; -w:抓包内容写入文件
tcpdump -nn -vv -i enp0s3 -c 1 -w baidu.pcap "dst host www.baidu.com"

# 读取包中的内容
tcpdump -r baidu.pcap
```

---

# libpcap API的简单使用

先安装下这个库。

```shell
# ubuntu
sudo apt install libpcap-dev
```

然后，我们来使用libpcap的API写写demo。

[tcpdump-libpcap](https://www.tcpdump.org/)中已经列出了多个教程。本节参考：

1. [PCAP(3PCAP) MAN PAGE](https://www.tcpdump.org/manpages/pcap.3pcap.html)：这个是官方的API文档。
2. [Using libpcap in C | DevDungeon](https://www.devdungeon.com/content/using-libpcap-c)：这个是一个很不错的libpcap API教程文档。内容循序渐进。美中不足的是，这篇文档是2015年写的，其中使用的一些API已经deprecate。

我这里滥竽充数的写一个libpcap的demo。这个demo展示了libpcap的使用骨架流程。结构性的了解了这些API的使用后，剩下的交给chatgpt即可。

下面是demo的流程：

- 先使用`pcap_create`创建一个句柄
- 使用`pcap_set_timeout`, 设置包缓冲区超时为1秒。避免等待过长时间：[c - Why is there a long delay between pcap\_loop() and getting a packet? - Stack Overflow](https://stackoverflow.com/questions/29875110/why-is-there-a-long-delay-between-pcap-loop-and-getting-a-packet)。
- 句柄上其他合适的选项，也是在`pcap_create`之后设置。
- 然后调用`pcap_activate`激活句柄。
- 使用`pcap_compile`将BPF的字符串表达式编译成伪机器码。
- `pcap_setfilter`将生成的伪机器码制设置为句柄的过滤器。
- `pcap_loop`读取数据包，并调用回调函数。
- `pcap_dump_*`则将数据包保存到文件系统中。
- `pcap_open_offline`则是加载PCAP格式的文件。后续的过滤显示等操作，同上。

下面是完整的代码。

```c
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_NAME "enp0s3"
#define PACKAGE_CAPTURE_COUNT 2
#define PCAP_DUMP_FILE_PATH "./baidu.pcap"

int enum_dev() {
  // 查看当前有哪些网络设备
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t *alldevs, *dev;

  // 获取可用的网络设备列表
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
    return -1;
  }

  int count = 1;
  struct sockaddr_in in;
  // 遍历设备列表并打印信息
  for (dev = alldevs; dev != NULL; dev = dev->next) {
    printf("Device %d: %s\n", count++, dev->name);
    if (dev->description) {
      printf("   Description: %s\n", dev->description);
    }
    for (pcap_addr_t *address = dev->addresses; address != NULL;
         address = address->next) {
      char addr_str[INET6_ADDRSTRLEN];
      const struct sockaddr *sa = address->addr;
      if (sa) {
        char addr_str[INET6_ADDRSTRLEN];
        if (sa->sa_family == AF_INET) {
          struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
          inet_ntop(AF_INET, &(ipv4->sin_addr), addr_str, sizeof(addr_str));
          printf("  IPv4 Address: %s\n", addr_str);
        } else if (sa->sa_family == AF_INET6) {
          struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
          inet_ntop(AF_INET6, &(ipv6->sin6_addr), addr_str, sizeof(addr_str));
          printf("  IPv6 Address: %s\n", addr_str);
        }
      }
    }
  }

  // 释放设备列表
  pcap_freealldevs(alldevs);
  printf("\n\n");
  return 0;
}

int print_device_net() {
  // 查看dev的 network number和子网掩码
  // ipv4变量中并非ip值，而是 network number
  // ref: https://github.com/the-tcpdump-group/libpcap/issues/159
  char errbuf[PCAP_ERRBUF_SIZE];
  int ret;
  bpf_u_int32 netp;
  bpf_u_int32 maskp;
  char ipv4[INET_ADDRSTRLEN];
  char ipv4_mask[INET_ADDRSTRLEN];
  ret = pcap_lookupnet(DEVICE_NAME, &netp, &maskp, errbuf);
  if (ret < 0) {
    fprintf(stderr, "fail in pcap_lookupnet: %s", errbuf);
    return -1;
  }
  inet_ntop(AF_INET, &netp, ipv4, sizeof(ipv4));
  inet_ntop(AF_INET, &maskp, ipv4_mask, sizeof(ipv4_mask));
  printf("%s network address is: %s\n", DEVICE_NAME, ipv4);
  printf("%s mask is: %s\n", DEVICE_NAME, ipv4_mask);
  printf("\n\n");
  return 0;
}

void print_mac_address(const u_int8_t *mac_address, char *mac_string,
                       unsigned int size) {
  snprintf(mac_string, size, "%02X:%02X:%02X:%02X:%02X:%02X", mac_address[0],
           mac_address[1], mac_address[2], mac_address[3], mac_address[4],
           mac_address[5]);
}

void print_packet_info(struct pcap_pkthdr packet_header, const u_char *packet) {
  printf("Packet capture length: %d\n", packet_header.caplen);
  printf("Packet total length %d\n", packet_header.len);

  // todo: 从const u_char *packet中，逐层解析
  char mac[ETH_HLEN + 5 + 1];
  struct ether_header *eth_header = (struct ether_header *)packet;
  print_mac_address(eth_header->ether_shost, mac, sizeof(mac));
  printf("src mac addr is: %s\n", mac);
  print_mac_address(eth_header->ether_dhost, mac, sizeof(mac));
  printf("dst mac addr is: %s\n", mac);

  if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
    struct iphdr *ip_header =
        (struct iphdr *)(packet + sizeof(struct ether_header));
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->saddr, ipv4, sizeof(ipv4));
    printf("src ip addr is: %s\n", ipv4);
    inet_ntop(AF_INET, &ip_header->daddr, ipv4, sizeof(ipv4));
    printf("dst ip addr is: %s\n", ipv4);
  }
}

void capture_packet_handler(u_char *args,
                            const struct pcap_pkthdr *packet_header,
                            const u_char *packet_body) {
  print_packet_info(*packet_header, packet_body);
  pcap_dump(args, packet_header, packet_body);
  return;
}

void capture() {
  // 捕获数据包,并保存到文件
  int ret;
  char errbuf[PCAP_ERRBUF_SIZE];

  pcap_t *pcap_handle;
  struct bpf_program filter;
  pcap_handle = pcap_create(DEVICE_NAME, errbuf);
  if (pcap_handle == NULL) {
    fprintf(stderr, "fail to execute pcap_create: %s", errbuf);
    exit(-1);
  }

  // https://stackoverflow.com/questions/29875110/why-is-there-a-long-delay-between-pcap-loop-and-getting-a-packet
  pcap_set_timeout(pcap_handle, 1000);

  ret = pcap_activate(pcap_handle);

  if (pcap_compile(pcap_handle, &filter, "dst host www.baidu.com", 0,
                   PCAP_NETMASK_UNKNOWN) < 0) {
    fprintf(stderr, "fail to compile bpf rule: %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }
  if (ret < 0) {
    fprintf(stderr, "fail to active pcap hanlde: %s", pcap_geterr(pcap_handle));
    exit(-1);
  }
  if (pcap_setfilter(pcap_handle, &filter) < 0) {
    printf("Error setting filter - %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }

  pcap_dumper_t *pacp_dump_point =
      pcap_dump_open(pcap_handle, PCAP_DUMP_FILE_PATH);
  if (pacp_dump_point == NULL) {
    printf("fail to open dump file - %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }

  pcap_loop(pcap_handle, PACKAGE_CAPTURE_COUNT, capture_packet_handler,
            (u_char *)pacp_dump_point);

  pcap_dump_flush(pacp_dump_point);
  printf("\n\nsaved package to %s\n\n", PCAP_DUMP_FILE_PATH);

  pcap_dump_close(pacp_dump_point);
  pcap_close(pcap_handle);
}

void load_packet_handler(u_char *args, const struct pcap_pkthdr *packet_header,
                         const u_char *packet_body) {
  print_packet_info(*packet_header, packet_body);
  return;
}

void load_pcap_file() {
  printf("load %s file.\n", PCAP_DUMP_FILE_PATH);
  char errbuf[PCAP_ERRBUF_SIZE];

  pcap_t *pcap_handle = pcap_open_offline(PCAP_DUMP_FILE_PATH, errbuf);
  if (pcap_handle == NULL) {
    fprintf(stderr, "fail to execute pcap_open_offline: %s", errbuf);
    exit(-1);
  }
  pcap_activate(pcap_handle);
  pcap_loop(pcap_handle, PACKAGE_CAPTURE_COUNT, load_packet_handler, NULL);
}

int main(int argc, char *argv[]) {
  printf("current libpcap version: %s\n", pcap_lib_version());
  capture();
  load_pcap_file();
}
```

输出如下：

```shell
current libpcap version: libpcap version 1.9.1 (with TPACKET_V3)
Packet capture length: 98
Packet total length 98
src mac addr is: 08:00:27:84:19:39
dst mac addr is: 80:12:DF:8F:5B:F4
src ip addr is: 192.168.18.131
dst ip addr is: 110.242.68.4
Packet capture length: 98
Packet total length 98
src mac addr is: 08:00:27:84:19:39
dst mac addr is: 80:12:DF:8F:5B:F4
src ip addr is: 192.168.18.131
dst ip addr is: 110.242.68.4


saved package to ./baidu.pcap

load ./baidu.pcap file.
Packet capture length: 98
Packet total length 98
src mac addr is: 08:00:27:84:19:39
dst mac addr is: 80:12:DF:8F:5B:F4
src ip addr is: 192.168.18.131
dst ip addr is: 110.242.68.4
Packet capture length: 98
Packet total length 98
src mac addr is: 08:00:27:84:19:39
dst mac addr is: 80:12:DF:8F:5B:F4
src ip addr is: 192.168.18.131
dst ip addr is: 110.242.68.4
```

---

# libpcap API的进阶使用

搞懂上面流程，然后根据实际需求编码即可。比如下面目标：
- 判断一个BPF表达式是否匹配一个在内存中的数据帧。使用`pcap_open_dead`+`pcap_offline_filter`很容易做到。
- [C语言使用libpcap输出报文到pcap文件\_pcap\_dump\_open-CSDN博客](https://blog.csdn.net/hyh19962008/article/details/119940985)