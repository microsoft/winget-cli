#pragma once

#include <string>

namespace valijson {
namespace internal {
namespace uri {

/**
  * @brief  Placeholder function to check whether a URI is absolute
  *
  * This function just checks for '://'
  */
inline bool isUriAbsolute(const std::string &documentUri)
{
    static const char * placeholderMarker = "://";

    return documentUri.find(placeholderMarker) != std::string::npos;
}

/**
  * Placeholder function to resolve a relative URI within a given scope
  */
inline std::string resolveRelativeUri(
        const std::string &resolutionScope,
        const std::string &relativeUri)
{
    return resolutionScope + relativeUri;
}

} // namespace uri
} // namespace internal
} // namespace valijson
