#include "directives.h"

namespace YAML {
Directives::Directives() : version{true, 1, 2}, tags{} {}

const std::string Directives::TranslateTagHandle(
    const std::string& handle) const {
  auto it = tags.find(handle);
  if (it == tags.end()) {
    if (handle == "!!")
      return "tag:yaml.org,2002:";
    return handle;
  }

  return it->second;
}
}  // namespace YAML
