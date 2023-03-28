[toc]

## 前言

JSON是一种轻量级的数据交换格式。程序员应该都知道的一种格式。最近我敲C++代码，使用了不同的C++ Json库。

这些JSON代码，完全可以交给chat-gpt去生成。但是，我们自身需要掌握其使用的一般规律。

本篇文章，**介绍不同C++ JSON库的一般使用规律**。

本篇使用不同的JSON C++库，对下面的JSON内容，进行构建和解析。

```json
{
    "name": "dacao",
    "description": "Block the following networks",
    "rules": [
        {
            "action": "block",
            "domain": "www.baidu.com",
            "ip": [
                "180.101.50.188",
                "180.101.50.242"
            ]
        }
    ]
}
```

---

## 背景介绍

[JSON](https://www.json.org/json-zh.html) 是一种轻量级的数据交换格式。 易于人阅读和编写。同时也易于机器解析和生成。JSON采用完全独立于语言的文本格式，但是也使用了类似于C语言家族的习惯（包括C, C++, C#, Java, JavaScript, Perl, Python等）。 这些特性使JSON成为理想的数据交换语言。

JSON对象是一个无序的“名称/值”集合。一个对象以左括号({)开始，以右括号(})结束。每个“名称”后跟一个冒号(:)。“名称/值”之间使用逗号(,)分隔。值（value）可以是双引号括起来的字符串（string）、数值(number)、true、false、 null、对象（object）或者数组（array）。这些结构可以嵌套。

![json](./json.png)

注：必须要理解上图含义，否则可能会理解不了下面编程的一般规律。

c++有很多json库。除了上面官网罗列了的大量JSON库，这里也可以看到C++ json库之间的比较：[nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark)
    
下面会尝试：Boost json、Qt json、nlohmann json，这三种json库。 

---

## C++ JSON库的使用

---
### Boost JSON

查看[Boost Version History](https://www.boost.org/users/history/)，我们知道[boost 1.75.0](https://www.boost.org/users/history/version_1_75_0.html)中引入[Boost.JSON](https://www.boost.org/doc/libs/1_79_0/libs/json/doc/html/index.html)库。

除了上面的官方文档，还可以参考：[BOOST的JSON解析库Boost.JSON简介](https://blog.csdn.net/alwaysrun/article/details/122158534)

参考[Document Model](https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom.html),boost::json文档中提供了四种模型。

* object: **具有唯一键的键值对的关联容器，其中键是字符串，映射类型是JSON值**。搜索、插入和删除具有平均的持续时间复杂度。此外，元素连续存储在内存中，允许缓存友好的迭代。
* value: 一种特殊的变体，**可以保存六种标准JSON数据类型中的任何一种**。
* array: JSON 值的序列容器，支持动态大小和快速、随机访问。接口和性能特征与. std::vector
* string: 连续的字符范围。该库假定字符串的内容仅包含有效的 UTF-8。

此时，对照上一节的背景介绍，我们可以**清晰的看到这些数据结构与json中的对应关系**。

* object：即使整个json结构的抽象。
* value：是“名称/值”中的值，它可以是不同的类型。
* array和string：是具体的类型之二。

是的，就这么简单。

下面是json构建的八字口诀：从上向下，从内向外。
* 从上向下：遇到花括号，创建一个object。遇到中括号，创建一个array。
    * object增加内容，使用operator[]添加新key，使用“=“赋值。
    * array增加内容，使用push_back添加内容。
* 从内向外：遇到嵌套结构，先构造里面内容，里面构造完后，赋值给外层。

至于JSON的解析规律，则更加简单。通过key，从object返回的结构是value，将其转换成六种标准中的一个，即可。

下面是示例代码，可以自行阅读。

```cpp
#include <boost/json.hpp>
#include <iostream>

std::string create_json() {
  // 构建json结构
  boost::json::object json_obj;
  json_obj["description"] = "boost json test";
  json_obj["name"] = "da1234cao";

  boost::json::array rules_arr;

  boost::json::object rule_obj;
  rule_obj["action"] = "block";
  rule_obj["domain"] = "www.baidu.com";

  boost::json::array ip_arr;
  ip_arr.push_back("180.101.50.188");
  ip_arr.push_back("180.101.50.242");
  rule_obj["ip"] = ip_arr;
  rules_arr.push_back(rule_obj);
  json_obj["rules"] = rules_arr;

  // 序列化
  std::string json_str = boost::json::serialize(json_obj);
  std::cout << json_str << std::endl;
  return json_str;
}

void parse_json(std::string json_str) {
  try {
    // 反列化
    boost::json::value json_val = boost::json::parse(json_str);
    boost::json::object re_json_obj = json_val.as_object();
    if (re_json_obj.contains("description")) {
      std::cout << "description: " << re_json_obj["description"].as_string()
                << std::endl;
    }

    if (re_json_obj.contains("name")) {
      std::cout << "name: " << re_json_obj["name"].as_string() << std::endl;
    }

    if (re_json_obj.contains("rules")) {
      boost::json::array &rules_arr = re_json_obj["rules"].as_array();
      for (int i = 0; i < rules_arr.size(); i++) {
        std::cout << "action: " << rules_arr[i].at("action").as_string()
                  << std::endl;
        std::cout << "domain: " << rules_arr[i].at("domain").as_string()
                  << std::endl;
        boost::json::array &ip_arr = rules_arr[i].at("ip").as_array();
        for (int j = 0; j < ip_arr.size(); j++) {
          std::cout << ip_arr[i].as_string() << std::endl;
        }
      }
    }
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::string json_str = create_json();
  parse_json(json_str);
  return 0;
}
```

---

### Qt JSON

参考文档：[JSON Support in Qt](https://doc.qt.io/qt-6/json.html)、[QT使用QJson生成解析json数据的方法](https://blog.csdn.net/qq_28351609/article/details/84892522)

Qt的json接口，与boost的json接口，区别不大。qt的使用QJsonDocument对象进行序列化和解析，且QJsonDocument解析后返回的是object，而不是value。

上一节的规律，在这节也使用。示例代码见下方。

```cpp
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

```

### nlohmann JSON

参考：[nlohmann/json](https://github.com/nlohmann/json)、[【C++ JSON 开源库】nlohmann入门使用总结 ](https://www.cnblogs.com/linuxAndMcu/p/14503341.html)

如果之前没使用过其他的C++ json库，nlohmann无疑是最容易上手json库之一了。在使用的过程中，它没有object,value,array这些概念。只需要nlohmann::json即可。

如果想创建一个name:string，直接使用赋值语句即可。如果想要创建key:array，调用push_bach方法。使用起来，非常丝滑。

```cpp
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

std::string create() {
  nlohmann::json block_json;
  block_json["name"] = "dacao";
  block_json["description"] = "nlohmann json test";

  nlohmann::json rules_json;
  nlohmann::json rule_json;
  rule_json["action"] = "block";
  rule_json["domain"] = "www.baidu.com";

  nlohmann::json ip_json;
  ip_json.push_back("180.101.50.188");
  ip_json.push_back("180.101.50.242");

  rule_json["ip"] = ip_json;
  rules_json.push_back(rule_json);
  block_json["rules"] = rules_json;

  std::string block_json_str = block_json.dump();
  std::cout << block_json << std::endl;
  return block_json_str;
}

void parse(std::string block_json_str) {
  try {
    nlohmann::json block_json = nlohmann::json::parse(block_json_str);
    std::cout << block_json.at("description").get<std::string>() << std::endl;
    if (block_json.contains("name")) {
      std::cout << block_json["name"].get<std::string>() << std::endl;
    }
    
    nlohmann::json &rules_json = block_json.at("rules");
    for(nlohmann::json &rule_json : rules_json) {
        std::cout << rule_json.at("action").get<std::string>() << std::endl;
        std::cout << rule_json.at("domain").get<std::string>() << std::endl;
        for(nlohmann::json &ip : rule_json["ip"]) {
            std::cout << ip.get<std::string>() << std::endl;
        }
    }

  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  parse(create());
  return 0;
}
```

