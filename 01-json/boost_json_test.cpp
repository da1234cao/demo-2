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
