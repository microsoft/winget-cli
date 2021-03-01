#pragma once

#include <iostream>

#include <Poco/JSON/JSONException.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, Poco::Dynamic::Var &document)
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
        document = Poco::JSON::Parser().parse(file);
    } catch (Poco::Exception const& exception) {
        std::cerr << "Poco::JSON failed to parse the document\n"
            << "Parse error:" << exception.what() << "\n";
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
