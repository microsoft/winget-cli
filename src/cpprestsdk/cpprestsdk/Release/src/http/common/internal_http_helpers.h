/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 ****/

#pragma once

#include "cpprest/asyncrt_utils.h"
#include "cpprest/details/basic_types.h"
#include <string>

namespace web
{
namespace http
{
namespace details
{
/// <summary>
/// Helper function to get the default HTTP reason phrase for a status code.
/// </summary>
utility::string_t get_default_reason_phrase(status_code code);

template<class Char, class Fn>
void trim_if(std::basic_string<Char>& str, Fn test)
{
    if (str.empty())
    {
        return;
    }

    auto first = str.begin();
    auto last = str.end();

    if (test(*first))
    {
        // removals at the front, and maybe the back
        for (;;)
        {
            ++first;
            if (first == last)
            {
                // all removals
                str.clear();
                return;
            }

            if (!test(*first))
            {
                break;
            }
        }

        do
        {
            --last;
        } while (test(*last));
        ++last;
        str.assign(first, last);
        return;
    }

    // no removals at the front, only maybe the back
    --last;
    if (!test(*last))
    {
        // no removals at all
        return;
    }

    do
    {
        --last;
    } while (test(*last));
    ++last;
    str.erase(last, str.end());
}

template<class Char>
void trim_nulls(std::basic_string<Char>& str)
{
    trim_if(str, [](const Char c) { return c == Char {}; });
}

template<class Char>
void trim_whitespace(std::basic_string<Char>& str)
{
    trim_if(str, [](const Char c) { return ::utility::details::is_space(c); });
}

bool validate_method(const utility::string_t& method);

} // namespace details
} // namespace http
} // namespace web

namespace web
{
namespace http
{
namespace compression
{
class decompress_factory;

namespace details
{
namespace builtin
{
/// <summary>
/// Helper function to get the set of built-in decompress factories
/// </summary>

const std::vector<std::shared_ptr<web::http::compression::decompress_factory>> get_decompress_factories();

} // namespace builtin
} // namespace details

} // namespace compression
} // namespace http
} // namespace web
