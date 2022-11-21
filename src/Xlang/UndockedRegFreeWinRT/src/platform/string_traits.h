#pragma once

#include <stdint.h>

namespace xlang::impl
{
    template <typename char_type>
    struct alternate_string_type;

    template <>
    struct alternate_string_type<xlang_char8>
    {
        using type = char16_t;
    };

    template <>
    struct alternate_string_type<char16_t>
    {
        using type = xlang_char8;
    };

    template <typename char_type>
    using alternate_string_type_t = typename alternate_string_type<char_type>::type;
}
