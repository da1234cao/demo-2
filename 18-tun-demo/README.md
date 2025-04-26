

```shell
cd linux
gcc ../simpletun.c -o simpletun

# 服务端
## 服务端ip为 192.168.1.4
./simpletun -i tun0 -s -d

# 服务端放行端口
## rock9
firewall-cmd --add-port=55555/tcp

# 客户端
./simpletun -i tun0 -c 192.168.1.4 -d
```