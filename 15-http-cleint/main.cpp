#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

bool request(QString url_str) {
  QUrl url(url_str);
  QNetworkRequest request(url);
  QNetworkAccessManager net_man;

  if (url.scheme() == "https") {
    QSslConfiguration sslConfiguration = request.sslConfiguration();
    sslConfiguration.setProtocol(QSsl::TlsV1_2OrLater);
    // 要求该证书是有效的
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(sslConfiguration);
  }

  QNetworkReply *reply = net_man.get(request);
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  request("https://www.baidu.com");
}