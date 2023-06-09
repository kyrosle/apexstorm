/**
 * @file test_config.cpp
 * @brief testing configuration module.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "config.h"
#include "log.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include <cstddef>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

// switching to enable test code
// #define TEST_CONFIG

#ifdef TEST_CONFIG

// Inital variables in config ----
apexstorm::ConfigVar<int>::ptr g_int_value_config =
    apexstorm::Config::Lookup("system.port", (int)8080, "system port");

apexstorm::ConfigVar<float>::ptr g_int_valuex_config =
    apexstorm::Config::Lookup("system.port", (float)8080, "system port");

apexstorm::ConfigVar<float>::ptr g_float_value_config =
    apexstorm::Config::Lookup("system.value", (float)10.2f, "system value");

apexstorm::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    apexstorm::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                              "system int vec");

apexstorm::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    apexstorm::Config::Lookup("system.int_list", std::list<int>{1, 2},
                              "system int list");

apexstorm::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    apexstorm::Config::Lookup("system.int_set", std::set<int>{1, 2},
                              "system int set");

apexstorm::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    apexstorm::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2},
                              "system int uset");

apexstorm::ConfigVar<std::map<std::string, int>>::ptr g_int_map_value_config =
    apexstorm::Config::Lookup("system.str_int_map",
                              std::map<std::string, int>{{"k", 2}},
                              "system str int map");

apexstorm::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_int_umap_value_config = apexstorm::Config::Lookup(
        "system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}},
        "system str int umap");

// ---- Inital variables in config

// recursive printing yaml node
void print_yaml(const YAML::Node &node, int level) {
  if (node.IsScalar()) {
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
        << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type()
        << " - " << level;
  } else if (node.IsNull()) {
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
        << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - "
        << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
          << std::string(level * 4, ' ') << it->first << " - "
          << it->second.Type() << " - " << level;
      print_yaml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      for (size_t i = 0; i < node.size(); ++i) {
        APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
            << std::string(level * 4, ' ') << i << " - " << node[i].Type()
            << " - " << level;
        print_yaml(node[i], level + 1);
      }
    }
  }
}

// test yaml config loading
void test_yaml() {
  YAML::Node root =
      YAML::LoadFile("/home/kyros/WorkStation/CPP/apexstorm/conf/log.yml");

  print_yaml(root, (int)apexstorm::LogLevel::Level::DEBUG);

  // APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT()) << root;
}

// test yaml config variable changed
void test_config() {
  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "before: " << g_int_value_config->getValue();
  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "before: " << g_float_value_config->getValue();

#define XX(g_var, name, prefix)                                                \
  {                                                                            \
    auto &v = g_var->getValue();                                               \
    for (auto &i : v) {                                                        \
      APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT()) << #prefix " " #name ": " << i; \
    }                                                                          \
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())                                   \
        << #prefix " " #name " yaml: " << g_var->toString();                   \
  }

#define XX_M(g_var, name, prefix)                                              \
  {                                                                            \
    auto &v = g_var->getValue();                                               \
    for (auto &i : v) {                                                        \
      APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())                                 \
          << #prefix " " #name ": {" << i.first << " - " << i.second << "}";   \
    }                                                                          \
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())                                   \
        << #prefix " " #name " yaml: " << g_var->toString();                   \
  }

  XX(g_int_vec_value_config, int_vec, before);
  XX(g_int_list_value_config, int_list, before);
  XX(g_int_set_value_config, int_set, before);
  XX(g_int_uset_value_config, int_uset, before);
  XX_M(g_int_map_value_config, str_int_map, before);
  XX_M(g_int_umap_value_config, str_int_umap, before);

  YAML::Node root =
      YAML::LoadFile("/home/kyros/WorkStation/CPP/apexstorm/conf/test.yml");
  apexstorm::Config::LoadFromYaml(root);

  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "after : " << g_int_value_config->getValue();
  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "after : " << g_float_value_config->getValue();

  XX(g_int_vec_value_config, int_vec, after);
  XX(g_int_list_value_config, int_list, after);
  XX(g_int_set_value_config, int_set, after);
  XX(g_int_uset_value_config, int_uset, after);
  XX_M(g_int_map_value_config, int_map, after);
  XX_M(g_int_umap_value_config, int_umap, after);
}

// support self defined data structures.
class Person {
public:
  std::string m_name;
  int m_age = 0;
  bool m_sex = 0;

  std::string toString() const {
    std::stringstream ss;
    ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex
       << "]";
    return ss.str();
  }

  // here: std::map required
  bool operator==(const Person &rhs) const {
    return m_name == rhs.m_name && m_age == rhs.m_age && m_sex == rhs.m_sex;
  }
};

// serialization and deserialization
namespace apexstorm {

template <> class LexicalCast<std::string, Person> {
public:
  Person operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    Person p;
    p.m_name = node["name"].as<std::string>();
    p.m_age = node["age"].as<int>();
    p.m_sex = node["sex"].as<bool>();
    return p;
  }
};

template <> class LexicalCast<Person, std::string> {
public:
  std::string operator()(const Person &v) {
    YAML::Node node;
    node["name"] = v.m_name;
    node["age"] = v.m_age;
    node["sex"] = v.m_sex;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

} // namespace apexstorm

apexstorm::ConfigVar<Person>::ptr g_person =
    apexstorm::Config::Lookup("class.person", Person(), "class person");

apexstorm::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    apexstorm::Config::Lookup("class.map", std::map<std::string, Person>(),
                              "class person map");

apexstorm::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr
    g_person_vec_map =
        apexstorm::Config::Lookup("class.vec_map",
                                  std::map<std::string, std::vector<Person>>(),
                                  "class person vec map");

void test_class() {
  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "before: " << g_person->getValue().toString() << " - "
      << g_person->toString();

#define XX_PM(g_var, prefix)                                                   \
  {                                                                            \
    auto m = g_var->getValue();                                                \
    for (auto &i : m) {                                                        \
      APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())                                 \
          << prefix << ": " << i.first << " - " << i.second.toString();        \
    }                                                                          \
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())                                   \
        << prefix << ": size=" << m.size();                                    \
  }

  g_person->addListener(10,
                        [](const Person &old_value, const Person &new_value) {
                          APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
                              << "old_value=" << old_value.toString()
                              << " new_value=" << new_value.toString();
                        });

  XX_PM(g_person_map, "class.map before");

  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "before: " << g_person_vec_map->toString();

  std::cout << std::endl;
  YAML::Node root =
      YAML::LoadFile("/home/kyros/WorkStation/CPP/apexstorm/conf/log.yml");
  apexstorm::Config::LoadFromYaml(root);

  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "after: " << g_person->getValue().toString() << " - "
      << g_person->toString();

  XX_PM(g_person_map, "class.map after");

  APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
      << "after: " << g_person_vec_map->toString();
}
#endif

// test log config loading and exporting
void test_log() {
  static apexstorm::Logger::ptr system_log = APEXSTORM_LOG_NAME("system");
  APEXSTORM_LOG_INFO(system_log) << "hello system" << std::endl;
  std::cout << apexstorm::LoggerMgr::GetInstance()->toYamlString() << std::endl;
  YAML::Node root =
      YAML::LoadFile("/home/kyros/WorkStation/CPP/apexstorm/conf/log.yml");
  apexstorm::Config::LoadFromYaml(root);
  std::cout << "=========================" << std::endl;
  std::cout << apexstorm::LoggerMgr::GetInstance()->toYamlString() << std::endl;
  APEXSTORM_LOG_INFO(system_log) << "hello system" << std::endl;

  apexstorm::Config::Visit([](apexstorm::ConfigVarBase::ptr var) {
    APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
        << var->getName() << " description=" << var->getDescription()
        << " typename=" << var->getTypeName() << " value=" << var->toString();
  });
}

int main() {
#ifdef TEST_CONFIG
  test_yaml();
  test_config();
  test_class();
#endif
  test_log();
}
