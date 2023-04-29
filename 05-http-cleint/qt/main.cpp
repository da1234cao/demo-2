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