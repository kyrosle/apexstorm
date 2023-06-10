#include "config.h"
#include "log.h"
#include "yaml-cpp/node/node.h"
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace apexstorm {

// Static Config Collection

// Config::ConfigVarMap Config::s_datas;
ConfigVarBase::~ConfigVarBase() {}

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
  RWMutexType::ReadLock lock(GetMutex());
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

// "A.B", 10
// A:
//  B: 10
//  C: str

// @param prefix prefix yaml path
// @param node YAML::Node structure
// @param[out] output identifier - node list collection
static void
ListAllMember(const std::string &prefix, const YAML::Node &node,
              std::list<std::pair<std::string, const YAML::Node>> &output) {
  // check yaml field invalidate
  if (prefix.find_first_not_of(VALID_CHAR) != std::string::npos) {
    APEXSTORM_LOG_ERROR(APEXSTORM_LOG_ROOT())
        << "Config invalid name: " << prefix << " : " << node;
  }

  output.push_back(std::make_pair(prefix, node));
  // recursively matching
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      // append . -> prefix.first
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto &i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);

    if (var) {
      // LexicalCast parsing
      if (i.second.IsScalar()) {
        var->fromString(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->fromString(ss.str());
      }
    }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  RWMutexType::ReadLock lock(GetMutex());
  ConfigVarMap &m = GetDatas();
  for (auto it = m.begin(); it != m.end(); ++it) {
    cb(it->second);
  }
}

} // namespace apexstorm
