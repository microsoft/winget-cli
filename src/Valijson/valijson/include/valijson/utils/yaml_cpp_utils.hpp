#pragma once

#include <iostream>
#include <memory>
#include <string>

#include <yaml-cpp/yaml.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, YAML::Node &document)
{
    try {
        document = YAML::LoadFile(path);
        return true;
    } catch (const YAML::BadFile &ex) {
        std::cerr << "Failed to load YAML from file '" << path << "'." << std::endl;
        return false;
    } catch (const YAML::ParserException &ex) {
        std::cout << "yaml-cpp failed to parse the document '" << ex.what() << std::endl;
        return false;
    }
}

} // namespace utils
} // namespace valijson
