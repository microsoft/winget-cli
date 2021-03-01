#pragma once

#include <iostream>

#include <picojson.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, picojson::value &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    // Parse schema
    std::string err = picojson::parse(document, file);
    if (!err.empty()) {
        std::cerr << "PicoJson failed to parse the document:" << std::endl
                  << "Parse error: " << err << std::endl;
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
