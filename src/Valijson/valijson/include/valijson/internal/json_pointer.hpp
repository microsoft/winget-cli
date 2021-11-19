#pragma once

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

#include <valijson/adapters/adapter.hpp>
#include <valijson/internal/optional.hpp>
#include <valijson/exceptions.hpp>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

namespace valijson {
namespace internal {
namespace json_pointer {

/**
 * @brief   Replace all occurrences of `search` with `replace`. Modifies `subject` in place.
 *
 * @param   subject  string to operate on
 * @param   search   string to search
 * @param   replace  replacement string
 */
inline void replaceAllInPlace(std::string& subject, const char* search,
                                const char* replace)
{
    size_t pos = 0;

    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, strlen(search), replace);
        pos += strlen(replace);
    }
}

/**
 * @brief   Return the char value corresponding to a 2-digit hexadecimal string
 *
 * @throws  std::runtime_error for strings that are not exactly two characters
 *          in length and for strings that contain non-hexadecimal characters
 *
 * @return  decoded char value corresponding to the hexadecimal string
 */
inline char decodePercentEncodedChar(const std::string &digits)
{
    if (digits.length() != 2) {
        throwRuntimeError("Failed to decode %-encoded character '" +
                digits + "' due to unexpected number of characters; "
                "expected two characters");
    }

    errno = 0;
    const char *begin = digits.c_str();
    char *end = nullptr;
    const unsigned long value = strtoul(begin, &end, 16);
    if (end != begin && *end != '\0') {
        throwRuntimeError("Failed to decode %-encoded character '" +
                digits + "'");
    }

    return char(value);
}

/**
 * @brief   Extract and transform the token between two iterators
 *
 * This function is responsible for extracting a JSON Reference token from
 * between two iterators, and performing any necessary transformations, before
 * returning the resulting string. Its main purpose is to replace the escaped
 * character sequences defined in the RFC-6901 (JSON Pointer), and to decode
 * %-encoded character sequences defined in RFC-3986 (URI).
 *
 * The encoding used in RFC-3986 should be familiar to many developers, but
 * the escaped character sequences used in JSON Pointers may be less so. From
 * the JSON Pointer specification (RFC 6901, April 2013):
 *
 *    Evaluation of each reference token begins by decoding any escaped
 *    character sequence.  This is performed by first transforming any
 *    occurrence of the sequence '~1' to '/', and then transforming any
 *    occurrence of the sequence '~0' to '~'.  By performing the
 *    substitutions in this order, an implementation avoids the error of
 *    turning '~01' first into '~1' and then into '/', which would be
 *    incorrect (the string '~01' correctly becomes '~1' after
 *    transformation).
 *
 * @param   begin  iterator pointing to beginning of a token
 * @param   end    iterator pointing to one character past the end of the token
 *
 * @return  string with escaped character sequences replaced
 *
 */
inline std::string extractReferenceToken(std::string::const_iterator begin,
        std::string::const_iterator end)
{
    std::string token(begin, end);

    // Replace JSON Pointer-specific escaped character sequences
    replaceAllInPlace(token, "~1", "/");
    replaceAllInPlace(token, "~0", "~");

    // Replace %-encoded character sequences with their actual characters
    for (size_t n = token.find('%'); n != std::string::npos;
            n = token.find('%', n + 1)) {

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            const char c = decodePercentEncodedChar(token.substr(n + 1, 2));
            token.replace(n, 3, &c, 1);
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::runtime_error &e) {
            throwRuntimeError(
                    std::string(e.what()) + "; in token: " + token);
        }
#endif
    }

    return token;
}

/**
 * @brief   Recursively locate the value referenced by a JSON Pointer
 *
 * This function takes both a string reference and an iterator to the beginning
 * of the substring that is being resolved. This iterator is expected to point
 * to the beginning of a reference token, whose length will be determined by
 * searching for the next delimiter ('/' or '\0'). A reference token must be
 * at least one character in length to be considered valid.
 *
 * Once the next reference token has been identified, it will be used either as
 * an array index or as an the name an object member. The validity of a
 * reference token depends on the type of the node currently being traversed,
 * and the applicability of the token to that node. For example, an array can
 * only be dereferenced by a non-negative integral index.
 *
 * Once the next node has been identified, the length of the remaining portion
 * of the JSON Pointer will be used to determine whether recursion should
 * terminate.
 *
 * @param   node            current node in recursive evaluation of JSON Pointer
 * @param   jsonPointer     string containing complete JSON Pointer
 * @param   jsonPointerItr  string iterator pointing the beginning of the next
 *                          reference token
 *
 * @return  an instance of AdapterType that wraps the dereferenced node
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &node,
        const std::string &jsonPointer,
        const std::string::const_iterator jsonPointerItr)
{
    // TODO: This function will probably need to implement support for
    // fetching documents referenced by JSON Pointers, similar to the
    // populateSchema function.

    const std::string::const_iterator jsonPointerEnd = jsonPointer.end();

    // Terminate recursion if all reference tokens have been consumed
    if (jsonPointerItr == jsonPointerEnd) {
        return node;
    }

    // Reference tokens must begin with a leading slash
    if (*jsonPointerItr != '/') {
        throwRuntimeError("Expected reference token to begin with "
                "leading slash; remaining tokens: " +
                std::string(jsonPointerItr, jsonPointerEnd));
    }

    // Find iterator that points to next slash or newline character; this is
    // one character past the end of the current reference token
    std::string::const_iterator jsonPointerNext =
            std::find(jsonPointerItr + 1, jsonPointerEnd, '/');

    // Extract the next reference token
    const std::string referenceToken = extractReferenceToken(
            jsonPointerItr + 1, jsonPointerNext);

    // Empty reference tokens should be ignored
    if (referenceToken.empty()) {
        return resolveJsonPointer(node, jsonPointer, jsonPointerNext);

    } else if (node.isArray()) {
        if (referenceToken == "-") {
            throwRuntimeError("Hyphens cannot be used as array indices "
                    "since the requested array element does not yet exist");
        }

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            // Fragment must be non-negative integer
            const uint64_t index = std::stoul(referenceToken);
            typedef typename AdapterType::Array Array;
            const Array arr = node.asArray();
            typename Array::const_iterator itr = arr.begin();
            const uint64_t arrSize = arr.size();

            if (arrSize == 0 || index > arrSize - 1) {
                throwRuntimeError("Expected reference token to identify "
                        "an element in the current array, but array index is "
                        "out of bounds; actual token: " + referenceToken);
            }

            if (index > static_cast<uint64_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
                throwRuntimeError("Array index out of bounds; hard "
                        "limit is " + std::to_string(
                                std::numeric_limits<std::ptrdiff_t>::max()));
            }

            itr.advance(static_cast<std::ptrdiff_t>(index));

            // Recursively process the remaining tokens
            return resolveJsonPointer(*itr, jsonPointer, jsonPointerNext);

#if VALIJSON_USE_EXCEPTIONS
        } catch (std::invalid_argument &) {
            throwRuntimeError("Expected reference token to contain a "
                    "non-negative integer to identify an element in the "
                    "current array; actual token: " + referenceToken);
        }
#endif
    } else if (node.maybeObject()) {
        // Fragment must identify a member of the candidate object
        typedef typename AdapterType::Object Object;

        const Object object = node.asObject();
        typename Object::const_iterator itr = object.find(
                referenceToken);
        if (itr == object.end()) {
            throwRuntimeError("Expected reference token to identify an "
                    "element in the current object; "
                    "actual token: " + referenceToken);
            abort();
        }

        // Recursively process the remaining tokens
        return resolveJsonPointer(itr->second, jsonPointer, jsonPointerNext);
    }

    throwRuntimeError("Expected end of JSON Pointer, but at least "
            "one reference token has not been processed; remaining tokens: " +
            std::string(jsonPointerNext, jsonPointerEnd));
    abort();
}

/**
 * @brief   Return the JSON Value referenced by a JSON Pointer
 *
 * @param   rootNode     node to use as root for JSON Pointer resolution
 * @param   jsonPointer  string containing JSON Pointer
 *
 * @return  an instance AdapterType in the specified document
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &rootNode,
        const std::string &jsonPointer)
{
    return resolveJsonPointer(rootNode, jsonPointer, jsonPointer.begin());
}

} // namespace json_pointer
} // namespace internal
} // namespace valijson

#ifdef _MSC_VER
#pragma warning( pop )
#endif
