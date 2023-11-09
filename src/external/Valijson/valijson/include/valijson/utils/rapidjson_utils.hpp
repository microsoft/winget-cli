#pragma once

#include <iostream>
#include <stdexcept>

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
#if VALIJSON_USE_EXCEPTIONS
    try {
#endif
        document.template Parse<rapidjson::kParseIterativeFlag>(file.c_str());
        if (document.HasParseError()) {
            std::cerr << "RapidJson failed to parse the document:" << std::endl;
            std::cerr << "Parse error: " << document.GetParseError() << std::endl;
            std::cerr << "Near: " << file.substr((std::max)(size_t(0), document.GetErrorOffset() - 20), 40) << std::endl;
            return false;
        }
#if VALIJSON_USE_EXCEPTIONS
    } catch (const std::runtime_error &e) {
        std::cerr << "RapidJson failed to parse the document:" << std::endl;
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson
