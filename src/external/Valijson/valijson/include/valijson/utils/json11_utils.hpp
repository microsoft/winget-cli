#pragma once

#include <iostream>

#include <json11.hpp>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, json11::Json &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    // Parse schema
    std::string err;
    document = json11::Json::parse(file, err);
    if (!err.empty()) {
        std::cerr << "json11 failed to parse the document:" << std::endl
                  << "Parse error: " << err << std::endl;
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
