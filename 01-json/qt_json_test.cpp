#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <iostream>

QString create_json() {
  QJsonObject json;
  json["name"] = "dacao";
  json["description"] = "Qt json test";

  QJsonArray rules;
  QJsonObject rule;
  rule["action"] = "block";
  rule["domain"] = "www.baidu.com";
  QJsonArray ip;
  ip.append("180.101.50.188");
  ip.append("180.101.50.242");
  rule["ip"] = ip;
  rules.append(rule);

  json["rules"] = rules;

  QJsonDocument doc(json);
  QString jsonString = doc.toJson(QJsonDocument::Compact);

  // Output the JSON string
  qDebug() << jsonString;
  return jsonString;
}

void parseJsonString(const QString &jsonString) {
  try {
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());

    if (doc.isNull()) {
      qDebug() << "Failed to parse JSON string.";
      return;
    }

    if (!doc.isObject()) {
      qDebug() << "JSON string is not an object.";
      return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("name"))
      qDebug() << obj["name"].toString();
    if (obj.contains("description"))
      qDebug() << obj["description"].toString();
    if (obj.contains("rules")) {
      QJsonArray rules_arr = obj["rules"].toArray();
      for (int i = 0; i < rules_arr.size(); i++) {
        QJsonObject rule = rules_arr[i].toObject();
        // Print "action" and "domain"
        qDebug() << "Action:" << rule["action"].toString();
        qDebug() << "Domain:" << rule["domain"].toString();

        // Get "ip" array
        QJsonArray ip = rule["ip"].toArray();

        // Iterate through "ip"
        for (int j = 0; j < ip.size(); j++) {
          qDebug() << "IP:" << ip[j].toString();
        }
      }
    }
  } catch (std::exception &e) {
    qDebug() << e.what();
  }
}
int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  parseJsonString(create_json());
}
