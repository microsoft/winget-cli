#pragma once

#include <iostream>

#include <json.hpp>
#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, nlohmann::json &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'."
                  << std::endl;
        return false;
    }

    // Parse schema
    try {
        document = nlohmann::json::parse(file);
    } catch (std::invalid_argument const& exception) {
        std::cerr << "nlohmann::json failed to parse the document\n"
            << "Parse error:" << exception.what() << "\n";
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
