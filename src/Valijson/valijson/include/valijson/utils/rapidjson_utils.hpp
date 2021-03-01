#pragma once

#include <iostream>

#include <rapidjson/document.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

template<typename Encoding, typename Allocator>
inline bool loadDocument(const std::string &path, rapidjson::GenericDocument<Encoding, Allocator> &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    // Parse schema
    document.template Parse<0>(file.c_str());
    if (document.HasParseError()) {
        std::cerr << "RapidJson failed to parse the document:" << std::endl;
        std::cerr << "Parse error: " << document.GetParseError() << std::endl;
        std::cerr << "Near: " << file.substr((std::max)(size_t(0), document.GetErrorOffset() - 20), 40) << std::endl;
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
