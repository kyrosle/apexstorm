#ifndef __APEXSTORM_CONFIG_H__
#define __APEXSTORM_CONFIG_H__

#include "log.h"
#include "util.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace apexstorm {

/// yaml field valid char set:
#define VALID_CHAR "abcdefghikjlmnopqrstuvwxyz._0123456789"

/**
 * @brief Base class for configuration variables
 */
class ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;

  /**
   * @brief Constructor
   * @param[in] name config name[0-9a-z_.]
   * @param[in] description config description
   */
  ConfigVarBase(const std::string &name, const std::string &description = "")
      : m_name(name), m_description(description) {
    // transform name into lower chars.
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }

  /**
   * @brief Destroy the Config Var Base object
   */
  virtual ~ConfigVarBase();

  /**
   * @brief Get the config name
   */
  const std::string &getName() const { return m_name; }

  /**
   * @brief Get the config description
   */
  const std::string &getDescription() const { return m_description; }

  /**
   * @brief Convert into string
   */
  virtual std::string toString() = 0;

  /**
   * @brief Initialize value from string
   */
  virtual bool fromString(const std::string &val) = 0;

  /**
   * @brief Returns the type name of the configuration parameter value
   */
  virtual std::string getTypeName() const = 0;

protected:
  std::string m_name;
  std::string m_description;
};

/**
 * @brief
 * @tparam F from_type
 * @tparam T to_type
 */
template <class F, class T> class LexicalCast {
public:
  T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

// ---- Container deviation specialization: std::vector

// from_type: std::string
// to_type  : std::vector<T>
template <class T> class LexicalCast<std::string, std::vector<T>> {
public:
  std::vector<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

// from_type: std::vector<T>
// to_type  : std::string
template <class T> class LexicalCast<std::vector<T>, std::string> {
public:
  std::string operator()(const std::vector<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::vector ----

// ---- Container deviation specialization: std::list

// from_type: std::string
// to_type  : std::list<T>
template <class T> class LexicalCast<std::string, std::list<T>> {
public:
  std::list<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

// from_type: std::list<T>
// to_type  : std::string
template <class T> class LexicalCast<std::list<T>, std::string> {
public:
  std::string operator()(const std::list<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::list ----

// ---- Container deviation specialization: std::set

// from_type: std::string
// to_type  : std::set<T>
template <class T> class LexicalCast<std::string, std::set<T>> {
public:
  std::set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

// from_type: std::set<T>
// to_type  : std::string
template <class T> class LexicalCast<std::set<T>, std::string> {
public:
  std::string operator()(const std::set<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::set ----

// ---- Container deviation specialization: std::unordered_set

// from_type: std::string
// to_type  : std::unordered_set<T>
template <class T> class LexicalCast<std::string, std::unordered_set<T>> {
public:
  std::unordered_set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

// from_type: std::unordered_set<T>
// to_type  : std::string
template <class T> class LexicalCast<std::unordered_set<T>, std::string> {
public:
  std::string operator()(const std::unordered_set<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::unordered_set ----

// ---- Container deviation specialization: std::map<std::string, T>

// from_type: std::string
// to_type  : std::map<std::string, T>
template <class T> class LexicalCast<std::string, std::map<std::string, T>> {
public:
  std::map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::map<std::string, T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return vec;
  }
};

// from_type: std::map<std::string, T>
// to_type  : std::string
template <class T> class LexicalCast<std::map<std::string, T>, std::string> {
public:
  std::string operator()(const std::map<std::string, T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
      // node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::map<std::string, T> ----

// ---- Container deviation specialization: std::unordered_map<std::string, T>

// from_type: std::string
// to_type  : std::unordered_map<std::string, T>
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
  std::unordered_map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_map<std::string, T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return vec;
  }
};

// from_type: std::unordered_map<std::string, T>
// to_type  : std::string
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
  std::string operator()(const std::unordered_map<std::string, T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
      // node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// Container deviation specialization: std::unordered_map<std::string, T> ----

/**
 * @brief
 * @tparam FromStr: T           operator(const std::string&)
 * @tparam ToStr  : std::string operator(const &T)
 */
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_value, const T &new_value)>
      on_change_callback;

  /**
   * @brief Constructor
   * @param  name             configuration name
   * @param  default_value    configuration default value
   * @param  description      configuration description
   */
  ConfigVar(const std::string &name, const T &default_value,
            const std::string &description = "")
      : ConfigVarBase(name, description), m_val(default_value) {}

  /**
   * @brief Convert configuration value into YAML string
   * @exception if converting meets error, throw exception
   */
  std::string toString() override {
    try {
      // return boost::lexical_cast<std::string>(m_val);
      return ToStr()(m_val);
    } catch (std::exception &e) {
      APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
          << "ConfigVar::toSting exception" << e.what()
          << " convert: " << typeid(m_val).name() << " to string";
    }
    return "";
  }

  /**
   * @brief Convert YAML string into configuration value
   * @exception if converting meets error, throw exception
   */
  bool fromString(const std::string &val) override {
    try {
      // m_val = boost::lexical_cast<T>(val);
      setValue(FromStr()(val));
      // return true;
    } catch (std::exception &e) {
      APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
          << "ConfigVar::fromString exception" << e.what()
          << " convert: string to " << typeid(m_val).name() << " - " << val;
    }
    return false;
  }

  /**
   * @brief Get the Value
   */
  const T getValue() const { return m_val; }

  /**
   * @brief Set the Value, if the value has changed, notify the callback
   * listener.
   */
  void setValue(const T &v) {
    if (v == m_val) {
      return;
    }
    for (auto &i : m_callbacks) {
      i.second(m_val, v);
    }
    m_val = v;
  }

  /**
   * @brief Returns the type name of the parameter value
   */
  std::string getTypeName() const override { return typeid(T).name(); }

  /**
   * @brief add callback listener
   * @param  key              the unique key, used to remove callback function
   * @param  cb               callback function
   */
  void addListener(uint64_t key, on_change_callback cb) {
    m_callbacks[key] = cb;
  }

  /**
   * @brief remove callback listener
   * @param  key              the unique key of callback function
   */
  void delListener(uint64_t key) { m_callbacks.erase(key); }

  /**
   * @brief clear all listeners
   */
  void clearListener() { m_callbacks.clear(); }

  /**
   * @brief Get the Listener
   * @param  key              the unqiue key of callback function
   * @return on_change_callback
   */
  on_change_callback getListener(uint64_t key) {
    auto it = m_callbacks.find(key);
    return it == m_callbacks.end() ? nullptr : it->second;
  }

private:
  T m_val;
  /// record callbacks, uint64_t unique key(hash)
  std::map<uint64_t, on_change_callback> m_callbacks;
};

/**
 * @brief ConfigVar Manager
 */
class Config {
public:
  typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

  /**
   * @brief Get/Create the correspond configuration name
   * @param  name             configuration name
   * @param  default_value    configuration default name
   * @param  description      configuration description
   */
  template <class T>
  static typename ConfigVar<T>::ptr
  Lookup(const std::string &name, const T &default_value,
         const std::string &description = "") {

    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp) {
        APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
            << "Lookup name=" << name << " exists";
        return tmp;
      } else {
        APEXSTORM_LOG_INFO(APEXSTORM_LOG_ROOT())
            << "Lookup name=" << name << " exists but type not "
            << typeid(T).name() << "real_type=" << it->second->getTypeName()
            << " " << it->second->toString();
        return nullptr;
      }
    }

    if (name.find_first_not_of(VALID_CHAR) != std::string::npos) {
      APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
          << "Lookup name invalid " << name;
      throw std::invalid_argument(name);
    }

    typename ConfigVar<T>::ptr v(
        new ConfigVar<T>(name, default_value, description));

    GetDatas()[name] = v;

    return v;
  }

  /**
   * @brief search configuration
   * @param  name             configurable name
   */
  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  /**
   * @brief load config from structure parsed by yaml-cpp
   */
  static void LoadFromYaml(const YAML::Node &root);

  /**
   * @brief search config by configuration name
   */
  static ConfigVarBase::ptr LookupBase(const std::string &name);

private:
  /**
   * @brief Return all configurations
   */
  static ConfigVarMap &GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }
};

} // namespace apexstorm

#endif
