[toc]

# C++发起https请求

## 前言

用c/c++发起一个https请求，不是一件容易的事情。

总的逻辑是这样：套接字的网络编程是基础；发送和接收的内容是http报文；为了保证安全，在tcp(网络层)和http(应用层)之间，插入TLS。

至于编程实现上面逻辑，在不同的库场景下，有不同的选择。
* 如果是桌面客户端编程，且公司购买了qt，最简单的方式，是使用[QT-HTTP Client](https://doc.qt.io/qt-6/qtnetwork-http-example.html)
* 如果不使用qt，且可以使用比较高版本的boost，可以使用[How to send a https request with boost beast](https://github.com/boostorg/beast/issues/825)
* 如果boost库都没法使用，而且是C++编程，可以去github上找些开源实现。
* 如果是C环境，不能使用C++。那就只好在使用套接字编程+openssl编程+http报文构造与解析(很麻烦)。可以参考：[Simple C example of doing an HTTP POST and consuming the response](https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response)、[how to do http & https request with openssl](http://jokinkuang.github.io/2015/02/27/how_to_do_http_&_https_request_with_openssl.html)

虽然很麻烦，但是我都实现了下。详细代码见仓库。

---

## qt发起https的get请求

qt对网络请求封装的很好，比较容易上手。但是商业版需要收费。

参考：[QT-HTTP Client](https://doc.qt.io/qt-6/qtnetwork-http-example.html)

```cpp
#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopeGuard>
#include <QSslKey>
#include <QUrl>

void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors) {
  qDebug() << "SSL verification errors:";
  for (const auto &error : errors) {
    qDebug() << "Error: " << error.errorString();
    const auto cert = error.certificate();
    if (!cert.isNull()) {
      qDebug() << "Issuer: " << cert.issuerInfo(QSslCertificate::CommonName);
      qDebug() << "Subject: " << cert.subjectInfo(QSslCertificate::CommonName);
      qDebug() << "Expiration date: " << cert.expiryDate().toString();
      if (cert.isBlacklisted()) {
        qDebug() << "Certificate is blacklisted!";
      }
      if (cert.publicKey().isNull()) {
        qDebug() << "Certificate has no public key!";
      }
    }
  }
  reply->ignoreSslErrors();
}

void http_get(QString url_str) {
  QUrl url(url_str);
  QNetworkRequest request;
  QNetworkAccessManager manager;

  if (url.scheme() == "https") {
    QSslConfiguration sslConfiguration = request.sslConfiguration();
    sslConfiguration.setProtocol(QSsl::TlsV1_2OrLater);
    // 要求该证书是有效的
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(sslConfiguration);
  }
  request.setUrl(url);

  QNetworkReply *reply = manager.get(request);
  QObject::connect(reply, &QNetworkReply::sslErrors, &QNetworkReply::sslErrors);
  auto guard = qScopeGuard([&reply] { reply->deleteLater(); });
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  qDebug()
      << "HTTP Status Code: "
      << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error() != QNetworkReply::NoError) {
    qDebug() << reply->errorString();
  } else {
    qDebug() << reply->readAll();
  }
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
  http_get("https://www.baidu.com/");
  // app.exec();
  return 0;
}
```

---

## beast发起https的get请求

beast要比qt难用些。中文示例很少，直接看官方的demo，可见下方链接。

参考：[How to send a https request with boost beast](https://github.com/boostorg/beast/issues/825)、[What do I need to do to make Boost.Beast HTTP parser find the end of the body?](https://stackoverflow.com/questions/66724298/what-do-i-need-to-do-to-make-boost-beast-http-parser-find-the-end-of-the-body)、[Using Boost-Beast (Asio) http client with SSL (HTTPS)](https://stackoverflow.com/questions/49507407/using-boost-beast-asio-http-client-with-ssl-https)

```cpp
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>

class req_info {
public:
  std::string host;
  std::string service;
  std::string target;
  boost::beast::http::verb method;
};

class request {
public:
  request() = default;
  request(req_info info_) : info(info_) {
    IS_HTTPS = info.service == "https" ? true : false;
  }

  void get_response() {
    if (IS_HTTPS) {
      https_get();
    } else {
      http_get();
    }
  }

private:
  void http_get() {
    try {
      req.method(info.method);
      req.target(info.target);
      req.set(boost::beast::http::field::host, info.host);
      req.set(boost::beast::http::field::user_agent, "boost beast test");
      boost::asio::io_context ioc;
      boost::asio::ip::tcp::resolver resolver(ioc);
      auto results = resolver.resolve(info.host, info.service);

      boost::beast::tcp_stream stream(ioc);
      stream.connect(results);

      boost::beast::http::write(stream, req);

      boost::beast::flat_buffer buffer;
      boost::beast::http::read(stream, buffer, res);

      std::cout << "return code: " << res.result_int() << std::endl;
      std::cout << "HTTP/" << res.version() << " " << res.result() << " "
                << res.reason() << "\n";
      std::cout << "Body: "
                << boost::beast::buffers_to_string(res.body().data()) << "\n";
      stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    } catch (std::exception const &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  void https_get() {
    try {
      boost::asio::io_context ioc;
      boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
      ctx.set_verify_mode(boost::asio::ssl::verify_none);
      boost::asio::ip::tcp::resolver resolver(ioc);
      auto results = resolver.resolve(info.host, info.service);

      boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);
      // Set SNI Hostname (many hosts need this to handshake successfully)
      SSL_set_tlsext_host_name(stream.native_handle(), info.host.c_str());

      boost::beast::get_lowest_layer(stream).connect(results);

      // Perform the SSL handshake
      stream.handshake(boost::asio::ssl::stream_base::client);

      req.method(info.method);
      req.target(info.target);
      req.set(boost::beast::http::field::host, info.host);
      req.set(boost::beast::http::field::user_agent, "boost beast https test");

      boost::beast::http::write(stream, req);

      boost::beast::flat_buffer buffer;
      boost::beast::http::read(stream, buffer, res);

      std::cout << "https response" << std::endl;
      std::cout << "return code: " << res.result_int() << std::endl;
      std::cout << "HTTP/" << res.version() << " " << res.result() << " "
                << res.reason() << "\n";
      std::cout << "Body: "
                << boost::beast::buffers_to_string(res.body().data()) << "\n";
      stream.shutdown();
    } catch (std::exception const &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

private:
  req_info info;
  bool IS_HTTPS;
  boost::beast::http::request<boost::beast::http::string_body> req;
  boost::beast::http::response<boost::beast::http::dynamic_body> res;
};

int main(int argc, char *argv[]) {
  req_info info;
  info.host = "www.baidu.com";
  // info.service = "http";
  info.service = "https";
  info.target = "/";
  info.method = boost::beast::http::verb::get;

  request req(info);
  req.get_response();
}
```

---

## socket+openssl+http_parse发起https的get请求

不到万不得已，不要使用此方案。太麻烦了。

参考：[Simple C example of doing an HTTP POST and consuming the response](https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response)、[C to perform HTTPS requests with openssl](https://stackoverflow.com/questions/62011930/c-to-perform-https-requests-with-openssl)、[how to do http & https request with openssl](http://jokinkuang.github.io/2015/02/27/how_to_do_http_&_https_request_with_openssl.html)

因为要同时支持http/https请求，所以使用了函数指针。在结构上参考了：[wrk2-sock](https://github.com/giltene/wrk2/blob/44a94c17d8e6a0bac8559b53da76848e430cb7a7/src/wrk.c#L34)

下面代码不全，详见仓库。

首先是需要一个存储连接的结构。

```cpp
typedef enum { OK, ERROR, RETRY } status;

typedef struct connection {
  http_parser parser;
  http_parser_settings settings;
  int fd;
  SSL_CTX *ctx;
  SSL *ssl;
  char url[URL_MAX_LENGTH];
  struct http_parser_url url_parts;
  char send_uf[SENDBUF];
  char recv_buf[RECVBUF];
  int recv_n;
  int is_recv_all;
} connection;
```

使用函数指针来统一http/https的连接过程。

```cpp
struct sock {
  status (*init)(connection *c);
  status (*connect)(connection *c);
  status (*read)(connection *c);
  status (*write)(connection *c);
  status (*close)(connection *c);
  size_t (*readable)(connection *c);
};
```

对http只需要socket+http_parse即可。

```cpp
#include "net.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

status socket_init(connection *con) {
  // do nothing
  return OK;
}

status socket_connect(connection *con) {
  int status;
  struct addrinfo hints;
  struct addrinfo *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;
  hints.ai_family = AF_INET; // AF_UNSPEC为 AF_INET or AF_INET6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP; // 指定协议为TCP

  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  char *schema = copy_url_part(con->url, &con->url_parts, UF_SCHEMA);
  if ((status = getaddrinfo(host, schema, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return ERROR;
  }

  for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
    int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
      con->fd = sockfd;
      break;
    }
  }

  free(host);
  free(schema);
  return OK;
}

status socket_write(connection *con) {
  int n = 0;
  while (n < strlen(con->send_uf)) {
    int send_n = write(con->fd, con->send_uf + n, strlen(con->send_uf) - n);
    if (send_n == -1) {
      return ERROR;
    }
    n += send_n;
  }
  return OK;
}

status socket_read(connection *con) {
  while (con->is_recv_all == 0 && (sizeof(con->recv_buf) - con->recv_n > 0)) {
    // 同步方式，当读取到一个完整的response的时候，停止
    int n = read(con->fd, con->recv_buf + con->recv_n,
                 sizeof(con->recv_buf) - con->recv_n);
    con->recv_n += n;
    http_parser_execute(&con->parser, &con->settings, con->recv_buf,
                        con->recv_n);
  }
  return OK;
}

status socket_close(connection *con) {
  if (con->fd > 0) {
    close(con->fd);
  }
}

size_t sock_readable(connection *c) {
  int n, rc;
  rc = ioctl(c->fd, FIONREAD, &n);
  return rc == -1 ? 0 : n;
}
```

对于https连接则需要引入openssl。

```cpp
#include "ssl.h"
#include "net.h"

status ssl_init(connection *con) {
  if (con->ctx = SSL_CTX_new(SSLv23_client_method())) {
    SSL_CTX_set_verify(con->ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_verify_depth(con->ctx, 0);
    SSL_CTX_set_mode(con->ctx, SSL_MODE_AUTO_RETRY);
    con->ssl = SSL_new(con->ctx);
    return OK;
  }
  return ERROR;
}

status ssl_connect(connection *con) {
  socket_connect(con);
  int r;
  SSL_set_fd(con->ssl, con->fd);
  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  SSL_set_tlsext_host_name(con->ssl, host);
  if ((r = SSL_connect(con->ssl)) != 1) {
    switch (SSL_get_error(con->ssl, r)) {
    case SSL_ERROR_WANT_READ:
      return RETRY;
    case SSL_ERROR_WANT_WRITE:
      return RETRY;
    default:
      return ERROR;
    }
  }
  return OK;
}

status ssl_read(connection *con) {
  while (con->is_recv_all == 0 && (sizeof(con->recv_buf) - con->recv_n > 0)) {
    int n = SSL_read(con->ssl, con->recv_buf, sizeof(con->recv_buf));
    if (n < 0) {
      switch (SSL_get_error(con->ssl, n)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        continue;
      default:
        return ERROR;
      }
    }
    con->recv_n += n;
    http_parser_execute(&con->parser, &con->settings, con->recv_buf,
                        con->recv_n);
  }
  return OK;
}

status ssl_write(connection *con) {
  int n = 0;
  while (n < strlen(con->send_uf)) {
    int send_n =
        SSL_write(con->ssl, con->send_uf + n, strlen(con->send_uf) - n);
    if (send_n < 0) {
      switch (SSL_get_error(con->ssl, send_n)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        continue;
      default:
        return ERROR;
      }
    }
    n += send_n;
  }
  return OK;
}

status ssl_close(connection *c) {
  SSL_shutdown(c->ssl);
  SSL_clear(c->ssl);
  return OK;
}

size_t ssl_readable(connection *c) { return SSL_pending(c->ssl); }
```