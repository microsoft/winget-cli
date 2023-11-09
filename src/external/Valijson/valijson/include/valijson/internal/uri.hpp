#pragma once

#include <regex>
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
 * @brief  Placeholder function to check whether a URI is a URN
 *
 * This function validates that the URI matches the RFC 8141 spec
 */
inline bool isUrn(const std::string &documentUri) {
  static const std::regex pattern(
      "^((urn)|(URN)):(?!urn:)([a-zA-Z0-9][a-zA-Z0-9-]{1,31})(:[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;=]+)+(\\?[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}(#[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}$");

  return std::regex_match(documentUri, pattern);
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
