#pragma once

#include <iostream>

#include <nlohmann/json.hpp>
#include <valijson/utils/file_utils.hpp>
#include <valijson/exceptions.hpp>

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
#if VALIJSON_USE_EXCEPTION
    try {
        document = nlohmann::json::parse(file);
    } catch (std::invalid_argument const& exception) {
        std::cerr << "nlohmann::json failed to parse the document\n"
            << "Parse error:" << exception.what() << "\n";
        return false;
    }
#else
    document = nlohmann::json::parse(file, nullptr, false);
    if (document.is_discarded()) {
        std::cerr << "nlohmann::json failed to parse the document.";
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson
