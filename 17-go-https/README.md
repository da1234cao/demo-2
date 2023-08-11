[toc]

# 目标

client ---握手--- https server

client ---请求--- /hello,/reflect

验证证书 -------- 证书
跳过    -------自签名证书

---

# 申请免费的证书

首先是安装[acme.sh](https://github.com/acmesh-official/acme.sh/wiki/%E8%AF%B4%E6%98%8E)

```shell
sudo apt-get -y install socat

da1234cao@vultr:~$ curl https://get.acme.sh | sh -s email=da1234cao@163.com
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  1032    0  1032    0     0   4384      0 --:--:-- --:--:-- --:--:--  4410
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100  216k  100  216k    0     0   715k      0 --:--:-- --:--:-- --:--:--  715k
[Fri Aug 11 06:17:18 AM UTC 2023] Installing from online archive.
[Fri Aug 11 06:17:18 AM UTC 2023] Downloading https://github.com/acmesh-official/acme.sh/archive/master.tar.gz
[Fri Aug 11 06:17:19 AM UTC 2023] Extracting master.tar.gz
[Fri Aug 11 06:17:19 AM UTC 2023] Installing to /home/da1234cao/.acme.sh
[Fri Aug 11 06:17:19 AM UTC 2023] Installed to /home/da1234cao/.acme.sh/acme.sh
[Fri Aug 11 06:17:19 AM UTC 2023] Installing alias to '/home/da1234cao/.bashrc'
[Fri Aug 11 06:17:19 AM UTC 2023] OK, Close and reopen your terminal to start using acme.sh
[Fri Aug 11 06:17:19 AM UTC 2023] Installing cron job
8 0 * * * "/home/da1234cao/.acme.sh"/acme.sh --cron --home "/home/da1234cao/.acme.sh" > /dev/null
[Fri Aug 11 06:17:19 AM UTC 2023] Good, bash is found, so change the shebang to use bash as preferred.
[Fri Aug 11 06:17:20 AM UTC 2023] OK
[Fri Aug 11 06:17:20 AM UTC 2023] Install success!
```

为了使用方便，我这里申请一个泛域名证书。我的域名是在阿里云购买的，所以本文仅尝试获取阿里云的泛域名证书。

参考：[阿里云域名使用ACME自动申请免费的通配符https域名证书](https://developers.weixin.qq.com/community/develop/article/doc/0008ae40ca0af83d0d7e3bb6b56013)、[acme.sh 使用泛域名|阿里云DNS |免费申请证书](https://blog.51cto.com/u_14131118/6066379)。

大概过程是：AccessKey管理->创建子用户->允许open API访问->添加DNS管理权限。将获取到的AccessKey 和 Secret 写到acme.sh.env配置文件里面。

```shell
export Ali_Key="*****“
export Ali_Secret=”*******"
```

执行`source ~/.bashrc`。

然后开始申请证书。

```shell
sudo ufw status
sudo ufw allow 80

# --debug 参数查看执行过程
# 没有web服务,80端口空闲, acme.sh 还能假装自己是一个webserver, 临时听在80 端口, 完成验证
## 如果执行报错;稍等等会再尝试;
acme.sh --issue --dns dns_ali -d *.da1234cao.top --standalone --debug

[Fri Aug 11 07:07:43 AM UTC 2023] Your cert is in: /home/da1234cao/.acme.sh/*.da1234cao.top_ecc/*.da1234cao.top.cer
[Fri Aug 11 07:07:43 AM UTC 2023] Your cert key is in: /home/da1234cao/.acme.sh/*.da1234cao.top_ecc/*.da1234cao.top.key
[Fri Aug 11 07:07:43 AM UTC 2023] The intermediate CA cert is in: /home/da1234cao/.acme.sh/*.da1234cao.top_ecc/ca.cer
[Fri Aug 11 07:07:43 AM UTC 2023] And the full chain certs is there: /home/da1234cao/.acme.sh/*.da1234cao.top_ecc/fullchain.cer
[Fri Aug 11 07:07:43 AM UTC 2023] _on_issue_success
```

```shell
# 查看证书信息
openssl x509 -noout -text -in '*.da1234cao.top.cer'
 Signature Algorithm: ecdsa-with-SHA384
        Issuer: C = AT, O = ZeroSSL, CN = ZeroSSL ECC Domain Secure Site CA
        Validity
            Not Before: Aug 11 00:00:00 2023 GMT
            Not After : Nov  9 23:59:59 2023 GMT
        Subject: CN = *.da1234cao.top  <-----

# 查看key信息
openssl ec -noout -text -in '*.da1234cao.top.key'
read EC key
Private-Key: (256 bit)
```