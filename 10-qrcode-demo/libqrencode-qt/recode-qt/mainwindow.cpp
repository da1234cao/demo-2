#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "qrencode.h"
#include <QBrush>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScopeGuard>

QPixmap MainWindow::GernerateQRCode(const QString &text, int scale) {
  QPixmap ret;
  int version = 0;
  QRcode *qrcode = QRcode_encodeString(text.toStdString().c_str(), version,
                                       QR_ECLEVEL_L, QR_MODE_8, 1);
  if (qrcode == NULL) {
    qDebug() << "Failed to encode the input data, errno is: " << errno;
    return ret;
  }
  auto cleanup = qScopeGuard([qrcode] { QRcode_free(qrcode); });

  if (qrcode->version != version) {
    qDebug() << "the input data too large";
    qDebug() << "change version from " << version << " to " << qrcode->version;
  }

  int width = qrcode->width * scale;
  int heigh = qrcode->width * scale;
  QImage image(width, heigh, QImage::Format_Mono);
  QPainter painter(&image);
  painter.fillRect(0, 0, width, heigh, Qt::white); // 背景填充白色
  painter.setPen(Qt::NoPen); // 填充但是不需要边界线
  painter.setBrush(QBrush(Qt::black, Qt::SolidPattern)); // 设置画笔为黑色
  for (int y = 0; y < qrcode->width; ++y) {
    for (int x = 0; x < qrcode->width; ++x) {
      if (qrcode->data[y * qrcode->width + x] & 0x01) {
        painter.drawRect(x * scale, y * scale, scale, scale);
      }
    }
  }
  ret = QPixmap::fromImage(image);
  return ret;
}

void MainWindow::onButtonClicked() {
  ui->button_qrcode->setEnabled(false);

  int width = ui->lab_qrcode->width();
  int heigh = ui->lab_qrcode->height();
  QString text = ui->textEdit_qrcode->toPlainText();
  if (!text.isEmpty()) {
    QPixmap pixmap = GernerateQRCode(text, 10);
    pixmap = pixmap.scaled(width, heigh, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
    ui->lab_qrcode->setPixmap(pixmap);
  }

  ui->button_qrcode->setEnabled(true);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  connect(ui->button_qrcode, &QPushButton::clicked, this,
          &MainWindow::onButtonClicked);
}

MainWindow::~MainWindow() { delete ui; }
