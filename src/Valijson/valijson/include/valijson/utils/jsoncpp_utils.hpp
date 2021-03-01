#pragma once

#include <iostream>

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

    Json::Reader reader;
    bool parsingSuccessful = reader.parse(file, document);
    if (!parsingSuccessful) {
        std::cerr << "Jsoncpp parser failed to parse the document:" << std::endl
                  << reader.getFormattedErrorMessages();
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
