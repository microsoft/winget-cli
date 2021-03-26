/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Implementation Details of the http.h layer of messaging
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "internal_http_helpers.h"

using namespace web;
using namespace utility;
using namespace utility::conversions;

namespace web
{
namespace http
{
namespace details
{
// Remove once VS 2013 is no longer supported.
#if defined(_WIN32) && _MSC_VER < 1900
static const http_status_to_phrase idToPhraseMap[] = {
#define _PHRASES
#define DAT(a, b, c) {status_codes::a, c},
#include "cpprest/details/http_constants.dat"
#undef _PHRASES
#undef DAT
};
#endif
utility::string_t get_default_reason_phrase(status_code code)
{
#if !defined(_WIN32) || _MSC_VER >= 1900
    // Future improvement: why is this stored as an array of structs instead of a map
    // indexed on the status code for faster lookup?
    // Not a big deal because it is uncommon to not include a reason phrase.
    static const http_status_to_phrase idToPhraseMap[] = {
#define _PHRASES
#define DAT(a, b, c) {status_codes::a, c},
#include "cpprest/details/http_constants.dat"
#undef _PHRASES
#undef DAT
    };
#endif

    utility::string_t phrase;
    for (const auto& elm : idToPhraseMap)
    {
        if (elm.id == code)
        {
            phrase = elm.phrase;
            break;
        }
    }
    return phrase;
}

size_t chunked_encoding::add_chunked_delimiters(_Out_writes_(buffer_size) uint8_t* data,
                                                _In_ size_t buffer_size,
                                                size_t bytes_read)
{
    size_t offset = 0;

    if (buffer_size < bytes_read + http::details::chunked_encoding::additional_encoding_space)
    {
        throw http_exception(_XPLATSTR("Insufficient buffer size."));
    }

    if (bytes_read == 0)
    {
        offset = 7;
        data[7] = '0';
        data[8] = '\r';
        data[9] = '\n'; // The end of the size.
        data[10] = '\r';
        data[11] = '\n'; // The end of the message.
    }
    else
    {
        char buffer[9];
#ifdef _WIN32
        sprintf_s(buffer, sizeof(buffer), "%8IX", bytes_read);
#else
        snprintf(buffer, sizeof(buffer), "%8zX", bytes_read);
#endif
        memcpy(&data[0], buffer, 8);
        while (data[offset] == ' ')
            ++offset;
        data[8] = '\r';
        data[9] = '\n'; // The end of the size.
        data[10 + bytes_read] = '\r';
        data[11 + bytes_read] = '\n'; // The end of the chunk.
    }

    return offset;
}

static const std::array<bool, 128> valid_chars = {{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0-15
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 16-31
    0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, // 32-47
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 48-63
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 64-79
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // 80-95
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 96-111
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0  // 112-127
}};

// Checks if the method contains any invalid characters
bool validate_method(const utility::string_t& method)
{
    for (const auto& ch : method)
    {
        size_t ch_sz = static_cast<size_t>(ch);
        if (ch_sz >= 128) return false;

        if (!valid_chars[ch_sz]) return false;
    }

    return true;
}

} // namespace details
} // namespace http
} // namespace web
