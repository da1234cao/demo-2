#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopeGuard>
#include <QUrl>

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
  auto guard = qScopeGuard([&reply] { reply->deleteLater(); });
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    qDebug() << reply->errorString();
  } else {
    qDebug() << reply->readAll();
  }
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
  http_get("https://www.example.com/");
  // app.exec();
  return 0;
}