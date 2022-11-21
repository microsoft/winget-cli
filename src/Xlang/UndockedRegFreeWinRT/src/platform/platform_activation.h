#pragma once

#include "pal.h"
#include <string_view>

namespace xlang::impl
{
#if XLANG_PLATFORM_WINDOWS
    using filesystem_char_type = char16_t;
#else
    using filesystem_char_type = char;
#endif

    inline constexpr std::string_view activation_fn_name{ "xlang_lib_get_activation_factory" };

    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<xlang_char8> module_namespace);

    xlang_pfn_lib_get_activation_factory try_get_activation_func(
        std::basic_string_view<char16_t> module_namespace);

    template <typename char_type>
    inline constexpr std::basic_string_view<char_type> enclosing_namespace(std::basic_string_view<char_type> str) noexcept
    {
        auto pos = str.rfind('.');
        if (pos == str.npos)
        {
            return {};
        }
        return str.substr(0, pos);
    }
}
