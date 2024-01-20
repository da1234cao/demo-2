[toc]

# dpdk网络转发环境的搭建

## ip命令的使用

参考自：
1. [ip 命令 - Router Lab 实验文档](https://lab.cs.tsinghua.edu.cn/router/doc/appendix/ip/#ip-link)
2. [ip(8) - Linux man page](https://linux.die.net/man/8/ip)

ip命令的总体组成如下。

```shell
ip [ OPTIONS ] OBJECT { COMMAND | help }

OBJECT := { link | addr | addrlabel | route | rule | neigh | tunnel | maddr | mroute | monitor }

OPTIONS := { -V[ersion] | -s[tatistics] | -r[esolve] | -f[amily] { inet | inet6 | ipx | dnet | link } | -o[neline] }
```

**ip address - protocol address management**.

每个设备必须有一个IP地址，才能使用对应的协议(IPV4/IPV6)。可以通过`ip address help`查看使用方法。

```shell
# 列出所有网口信息和地址信息
ip address show

# 设置网络
ip addr add $addr/$prefix_len dev $interface
```

**ip link - network device configuration**。

使用`ip link`来显示和修改网络设备的状态。具体使用方法，可以通过help查看。

```shell
# 查看设备状态
ip link show

# 创建两个虚拟以太网设备，它们之间直接相连
## ref: https://man7.org/linux/man-pages/man4/veth.4.html
### 在一对设备中的一个设备上传输的数据包会立即在其他设备上收到。当任一设备出现故障时，该对的链路状态为关闭。
### 这两个veth可以处在不同的网络命名空间中
## 如果有天希望三个veth可以互通，这似乎有点麻烦，我还没搞明白：https://superuser.com/questions/764986/howto-setup-a-veth-virtual-network
## 这里还有篇veth-pair配置的不错的介绍：https://www.cnblogs.com/bakari/p/10613710.html
ip link add $name1 type veth peer name $name2
```

**ip route - routing table management**.

操纵路由表。

```shell
# 查看路由表
ip route show
```

## 配置dpdk-basicfwd需要的网络结构

先设置一对虚拟以太网卡，并设置IP/mask。注意此时这两者无法互相ping通，但是可以通过lo口互通的，见:  [Linux 虚拟网络设备 veth-pair 详解，看这一篇就够了 - bakari - 博客园](https://www.cnblogs.com/bakari/p/10613710.html)

```shell
ip link add veth1 type veth peer name veth2
ip link set veth1 up
ip link set veth2 up
ip address add 10.0.0.2/24 dev veth1
ip address add 10.0.0.3/24 dev veth2

#测试下上面的配置是否可以联通。
## -l表示listen; -s表示veth2在80开启监听端口; 
## -k表示处理完一个连接后继续监听新的连接，而不是退出
nc -k -l -s 10.0.0.3 -p 80

## 从10.0.0.2发出流量到10.0.0.3:80 端口
echo "hello world" |  nc -s 10.0.0.2 -w 1 10.0.0.3 80
```

接着，我们再设置另一对虚拟以太网。

```shell
ip link add veth3 type veth peer name veth4
ip link set veth3 up
ip link set veth4 up
ip address add 172.16.0.2/24 dev veth3
ip address add 172.16.0.3/24 dev veth4
```

为了避免veth1和veth4通过lo口进行通信。将veth1和veth4放在不同的命名空间中。

```shell
# 添加两个命名空间
ip netns add nsA
ip netns add nsB

# 将veth1加入命名空间nsA; 然后进入命名空间,设置ip并启用
## 此时veth1能ping通veth2了
ip link set veth1 netns nsA
ip netns exec nsA  /bin/bash
ip link set veth1 up
ip address add 10.0.0.2/24 dev veth1
ping 10.0.0.3

# 将veth4加入命名空间nsB; 然后进入命名空间,设置ip并启用
## 此时veth4能ping通veth3了
ip link set veth4 netns nsB
ip netns exec nsB  /bin/bash
ip link set veth4 up
ip address add 172.16.0.3/24 dev veth4
ping 172.16.0.2
```


最终的结果：
* veth1和veth2可以通过veth-pair跨namespcae通信。
* veth3和veth4可以通过veth-pair跨namespcae通信。
* 但是veth1和veth2由于不在同一个namespace,又没有veth-pair，所以相互之间无法通信。

## 测试dpdk-basicfwd

