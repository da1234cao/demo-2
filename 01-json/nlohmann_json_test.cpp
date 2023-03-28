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