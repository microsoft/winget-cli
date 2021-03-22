#pragma once

#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wshorten-64-to-32"
# include <boost/property_tree/json_parser.hpp>
# pragma clang diagnostic pop
#else
# include <boost/property_tree/json_parser.hpp>
#endif

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, boost::property_tree::ptree &document)
{
    try {
        boost::property_tree::read_json(path, document);
    } catch (std::exception &e) {
        std::cerr << "Boost Property Tree JSON parser failed to parse the document:" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
