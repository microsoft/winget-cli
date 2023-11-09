#pragma once

#include <iostream>
#include <string>
#include <memory>

#include <json/json.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, Json::Value &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    const auto fileLength = static_cast<int>(file.length());
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string err;
    if (!reader->parse(file.c_str(), file.c_str() + fileLength, &document, &err)) {
        std::cerr << "Jsoncpp parser failed to parse the document:" << std::endl << err;
        return false;
    }
    return true;
}

}  // namespace utils
}  // namespace valijson
