#pragma once

#include <iostream>

#include <boost/json.hpp>
#include <valijson/utils/file_utils.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, boost::json::value &document)
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
#endif
      boost::json::error_code errorCode;
      boost::json::string_view stringView{file};
      document = boost::json::parse(stringView, errorCode);
        if (errorCode) {
            std::cerr << "Boost.JSON parsing error: " << errorCode.message();
            return false;
        }
#if VALIJSON_USE_EXCEPTION
    } catch (std::exception const & exception) {
        std::cerr << "Boost.JSON parsing exception: " << exception.what();
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson
