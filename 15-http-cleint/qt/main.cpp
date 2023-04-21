#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QLoggingCategory>

bool request(QString url_str, QString &err_msg, int timeout = 1000) {
  QUrl url(url_str);
  QNetworkRequest request;
  QNetworkAccessManager net_man;

  if (url.scheme() == "https") {
    QSslConfiguration sslConfiguration = request.sslConfiguration();
    sslConfiguration.setProtocol(QSsl::TlsV1_2OrLater);
    // 要求该证书是有效的
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(sslConfiguration);
  }
  request.setUrl(url);

  bool result = false;
  QNetworkReply *reply = net_man.get(request);
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() == QNetworkReply::NoError) {
    result = true;
  } else {
    err_msg = reply->errorString();
  }

  reply->deleteLater();

  return result;
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
  QString err_msg;
  bool ret = request("https://www.baidu.com", err_msg);
  if (ret == true) {
    qDebug() << "url is valid";
  } else {
    qDebug() << "url is not valid: " << err_msg;
  }

  // app.exec();
  return 0;
}