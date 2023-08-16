[toc]

# 前言

在公网中，我想加密传输的数据。(1)很自然，我想到了把数据放到http的请求中，然后通过tls确保数据安全。(2)更进一步，只要数据可以解析，则无需http协议，直接通过tls协议加密传输即可。本文分别尝试了这两个方案。

尝试实现方案之前，我们考虑需要实现哪些内容。(1)如何获取证书。(2)golang中如何实现一个https的客户端和服务器。(3)golang中如何实现一个tls的客户端和服务器。(4)http的request和response的构建,发送和解析。(5)对于客户端, 应用层(http)是否应该复用网络层(tcp)的连接; 哪些需求下不能复用; (6)不考虑传输层的网络细节。

注：本文不涉及相关内容的背景知识介绍。本文完整代码见仓库。

---

# 生成证书

如果有已经购买的域名，可以申请一个免费的通配符证书，便于日常使用。

没有域名的话：可以通过命令行生成证书，见：[windows和linux上证书的增删查](https://da1234cao.blog.csdn.net/article/details/130307635)。也可以通过go代码来创建证书。

---

## 申请免费的证书

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
export Ali_Key="*****"
export Ali_Secret="*******"
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

---

## 使用Go语言生成自签CA证书

这里有比较详细的介绍：[使用Go语言生成自签CA证书](https://foreverz.cn/go-cert)

```go
package certificate

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"io/ioutil"
	"math/big"
	"time"
)

func Gencertificate(output string) error {
	// ref: https://foreverz.cn/go-cert

	// 生成私钥
	priv, err := rsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		return err
	}

	// x509证书内容
	var csr = &x509.Certificate{
		Version:      3,
		SerialNumber: big.NewInt(time.Now().Unix()),
		Subject: pkix.Name{
			Country:            []string{"CN"},
			Province:           []string{"Shanghai"},
			Locality:           []string{"Shanghai"},
			Organization:       []string{"httpsDemo"},
			OrganizationalUnit: []string{"httpsDemo"},
			CommonName:         "da1234cao.top",
		},
		NotBefore:             time.Now(),
		NotAfter:              time.Now().AddDate(1, 0, 0),
		BasicConstraintsValid: true,
		IsCA:                  false,
		KeyUsage:              x509.KeyUsageDigitalSignature | x509.KeyUsageKeyEncipherment,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
	}

	// 证书签名
	certDer, err := x509.CreateCertificate(rand.Reader, csr, csr, priv.Public(), priv)
	if err != nil {
		return err
	}

	// 二进制证书解析
	interCert, err := x509.ParseCertificate(certDer)
	if err != nil {
		return err
	}

	// 证书写入文件
	pemData := pem.EncodeToMemory(&pem.Block{
		Type:  "CERTIFICATE",
		Bytes: interCert.Raw,
	})
	if err = ioutil.WriteFile(output+"cert.pem", pemData, 0644); err != nil {
		panic(err)
	}

	// 私钥写入文件
	keyData := pem.EncodeToMemory(&pem.Block{
		Type:  "EC PRIVATE KEY",
		Bytes: x509.MarshalPKCS1PrivateKey(priv),
	})

	if err = ioutil.WriteFile(output+"key.pem", keyData, 0644); err != nil {
		return err
	}

	return nil
}
```

---

# https的客户端和服务端

轮子已经有了,[net/http](https://pkg.go.dev/net/http)。我没有看到`net/http`很好的入门教程。只能看下官方文档，网上翻翻一些简单的示例。

下面是一个示例。其中，必须一提的是，http请求结束后，连接可以仍然存在，放到闲置的连接池中。便于后续请求，复用之前的连接。我到源码里面去看了下，没太看懂，可见：[Golang Http RoundTrip解析](https://blog.csdn.net/zhanglehes/article/details/122213793)。

---

## 服务端代码

启动服务端后，对于`/reflect`路径的request，构建一个response。注意其中的header和body内容的填充。

```go
func reflect(w http.ResponseWriter, r *http.Request) {
	log.Println("handle reflect")
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	bodyByte, _ := io.ReadAll(r.Body)
	log.Println("recv:", string(bodyByte))
	w.Write(bodyByte)
}

func Start() error {
	listenPort := Conf.ListenPort
	listenIp := Conf.ListenIp
	if listenPort <= 0 || listenPort > 65535 {
		log.Println("invalid listen port:", listenPort)
		return errors.New("invalid listen port")
	}

	http.HandleFunc("/reflect", reflect)
	err := http.ListenAndServeTLS(listenIp+":"+strconv.Itoa(listenPort), Conf.Protocol.Https.Certificate, Conf.Protocol.Https.Key, nil)
	return err
}
```

## 客户端代码

客户端可以选择是否验证服务端的证书。验证证书不重要，因为我信任这个域名解析过程(如果DNS没有被污染的话)。数据可以加密传输即可。代码中使用`http.NewRequest`构建一个请求，然后通过`http.Client.Do`发送请求(可能会复用之前的连接)。

```go
func New() *http.Client {
	cli := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{InsecureSkipVerify: Conf.SkipVerify},
		},
	}
	return cli
}

func DoRequest(cli *http.Client, data []byte) (*http.Response, error) {
	req, err := http.NewRequest("POST", Conf.Protocol+"://"+Conf.ServerIp+":"+strconv.Itoa(Conf.ServerPort)+"/reflect", bytes.NewBuffer(data))
	if err != nil {
		log.Println("fail to consstruct request", err)
	}
	return cli.Do(req)
}

func PrintResponse(resp *http.Response, err error) {
	if err == nil {
		if resp.StatusCode == http.StatusOK {
			body, _ := ioutil.ReadAll(resp.Body)
			log.Print(string(body))
		}
		log.Println("http status code:", resp.StatusCode)
	} else {
		log.Print(err)
	}
}
```

---

# tls的客户端和服务端

当我们的应用层不需要http协议，只需要对应用层的数据进行加密传输。我们尝试下面的代码(下面代码中，我手动构建了http的request和response,是为了保证接收到完整的数据后再处理,仅此而已)。 使用的库是[crypto/tls](https://pkg.go.dev/crypto/tls)

## 服务端

```go
func TLSDataHandle(conn net.Conn) {
	for {
		// 读取request
		ioBuf := bufio.NewReader(conn)
		req, err := http.ReadRequest(ioBuf)
		if err != nil {
			log.Println(err)
			return
		}
		defer req.Body.Close()
		defer conn.Close()
		bodyByte, _ := io.ReadAll(req.Body)
		log.Println("recv: ", string(bodyByte))

		// 构建一个response
		buf := bytes.NewBuffer(nil)
		buf.WriteString("HTTP/1.1 200 OK\r\n")
		buf.WriteString("Content-Length: " + strconv.Itoa(len(bodyByte)) + "\r\n")
		buf.WriteString("\r\n")
		buf.Write(bodyByte)

		// 发送response
		buf.WriteTo(conn)
	}
}

func TLSStart() error {
	listenPort := Conf.ListenPort
	listenIp := Conf.ListenIp
	if listenPort <= 0 || listenPort > 65535 {
		log.Println("invalid listen port:", listenPort)
		return errors.New("invalid listen port")
	}

	cert, err := tls.LoadX509KeyPair(Conf.Protocol.Https.Certificate, Conf.Protocol.Https.Key)
	if err != nil {
		log.Println("fail to laod x509 key pair", err)
	}

	config := &tls.Config{Certificates: []tls.Certificate{cert}}
	listener, _ := tls.Listen("tcp", listenIp+":"+strconv.Itoa(listenPort), config)
	for {
		conn, _ := listener.Accept()
		go TLSDataHandle(conn)
	}
}
```

## 客户端

```go
func NewTlsConn() (net.Conn, error) {
	config := &tls.Config{InsecureSkipVerify: Conf.SkipVerify}
	return tls.Dial("tcp", Conf.ServerIp+":"+strconv.Itoa(Conf.ServerPort), config)
}

func SendRequest(conn net.Conn, data []byte) {
	// 构造一个请求
	buf := bytes.NewBuffer(nil)
	buf.WriteString("POST /no_thing")
	buf.WriteString(" HTTP/1.1\r\n")
	buf.WriteString("Content-Length: " + strconv.Itoa(len(data)) + "\r\n")
	buf.WriteString("\r\n")
	buf.Write(data)

	// 发送请求
	buf.WriteTo(conn)

	// 读取回复
	ioBuf := bufio.NewReader(conn)
	res, err := http.ReadResponse(ioBuf, nil)
	if err != nil {
		log.Println(err)
		return
	}
	defer res.Body.Close()
	bodyByte, _ := io.ReadAll(res.Body)
	log.Println(string(bodyByte))
}
```