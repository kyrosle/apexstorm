/**
 * @file config.h
 * @brief Configuration module.
 * @author kyros (le@90e.com)
 * @version 1.0
 * @date 2023-06-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __APEXSTORM_CONFIG_H__
#define __APEXSTORM_CONFIG_H__

#include "log.h"
#include "thread.h"
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
#include <pthread.h>
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

// yaml field valid char set:
#define VALID_CHAR "abcdefghikjlmnopqrstuvwxyz._0123456789"

// Base class for configuration variables
class ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;

  // Constructor
  // @param name config name[0-9a-z_.]
  // @param description config description
  ConfigVarBase(const std::string &name, const std::string &description = "")
      : m_name(name), m_description(description) {
    // transform name into lower chars.
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }

  // Destroy the Config Var Base object
  virtual ~ConfigVarBase();

  // Get the Name object
  const std::string &getName() const { return m_name; }

  // Get the config description
  const std::string &getDescription() const { return m_description; }

  // Convert into string
  virtual std::string toString() = 0;

  // Initialize value from string
  virtual bool fromString(const std::string &val) = 0;

  // Returns the type name of the configuration parameter value
  virtual std::string getTypeName() const = 0;

protected:
  // configure variables name
  std::string m_name;
  // configure variables description
  std::string m_description;
};

// @tparam F from_type
// @tparam T to_type
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

// Configuration variables, using RWLock.
// @tparam FromStr: T           operator(const std::string&)
// @tparam ToStr  : std::string operator(const &T)
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
  typedef RWMutex RWMutexType;
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_value, const T &new_value)>
      on_change_callback;

  // Constructor
  // @param  name             configuration name
  // @param  default_value    configuration default value
  // @param  description      configuration description
  ConfigVar(const std::string &name, const T &default_value,
            const std::string &description = "")
      : ConfigVarBase(name, description), m_val(default_value) {}

  // Convert configuration value into YAML string (thread safety).
  // @exception if converting meets error, throw exception
  std::string toString() override {
    try {
      // return boost::lexical_cast<std::string>(m_val);
      RWMutexType::ReadLock lock(m_mutex);
      return ToStr()(m_val);
    } catch (std::exception &e) {
      APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
          << "ConfigVar::toSting exception" << e.what()
          << " convert: " << typeid(m_val).name() << " to string";
    }
    return "";
  }

  // Convert YAML string into configuration value.
  // @exception if converting meets error, throw exception
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

  // Get the Value (thread safety).
  const T getValue() {
    RWMutexType::ReadLock lock(m_mutex);
    return m_val;
  }

  // Set the Value, if the value has changed, notify the callback
  // listener (thread safety).
  void setValue(const T &v) {
    {
      RWMutexType::ReadLock lock(m_mutex);
      if (v == m_val) {
        return;
      }
      for (auto &i : m_callbacks) {
        i.second(m_val, v);
      }
    }

    RWMutexType::WriteLock lock(m_mutex);
    m_val = v;
  }

  // Returns the type name of the parameter value.
  std::string getTypeName() const override { return typeid(T).name(); }

  // add callback listener (thread safety).
  // @param  key              the unique key, used to remove callback function
  // @param  cb               callback function
  u_int64_t addListener(on_change_callback cb) {
    static uint64_t s_fun_id = 0;
    RWMutexType::WriteLock lock(m_mutex);
    m_callbacks[++s_fun_id] = cb;
    return s_fun_id;
  }

  // Remove callback listener (thread safety).
  // @param  key              the unique key of callback function
  void delListener(uint64_t key) {
    RWMutexType::WriteLock lock(m_mutex);
    m_callbacks.erase(key);
  }

  // Clear all listeners (thread safety).
  void clearListener() {
    RWMutexType::WriteLock lock(m_mutex);
    m_callbacks.clear();
  }

  // Get the Listener (thread safety).
  // @param  key              the unique key of callback function
  // @return on_change_callback
  on_change_callback getListener(uint64_t key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_callbacks.find(key);
    return it == m_callbacks.end() ? nullptr : it->second;
  }

private:
  T m_val;
  // record callbacks, uint64_t unique key(hash)
  std::map<uint64_t, on_change_callback> m_callbacks;
  // RWMutex structure
  RWMutexType m_mutex;
};

// ConfigVar Manager, using RWMutex.
class Config {
public:
  typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  typedef RWMutex RWMutexType;

  // Get/Create the correspond configuration name (thread safety).
  // @param  name             configuration name
  // @param  default_value    configuration default name
  // @param  description      configuration description
  template <class T>
  static typename ConfigVar<T>::ptr
  Lookup(const std::string &name, const T &default_value,
         const std::string &description = "") {

    RWMutexType::WriteLock lock(GetMutex());
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

  // Search configuration (thread safety).
  // @param  name             configurable name
  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  // load config from structure parsed by yaml-cpp

  static void LoadFromYaml(const YAML::Node &root);

  // search config by configuration name

  static ConfigVarBase::ptr LookupBase(const std::string &name);

  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
  // Return all configurations.
  static ConfigVarMap &GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }

  // Return the rwlock.
  static RWMutexType &GetMutex() {
    static RWMutexType s_mutex;
    return s_mutex;
  }
};

} // namespace apexstorm

#endif
