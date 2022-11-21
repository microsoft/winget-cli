#pragma once

#include "pal_error.h"

namespace xlang::impl
{
    // packed_buffer_size and get_packed_buffer_ptr perform calculations
    // that allow allocating a string control type and a backing character buffer
    // into a single block.
    template <typename class_type, typename char_type>
    inline uint32_t packed_buffer_size(uint32_t string_length)
    {
        constexpr auto max_size = std::numeric_limits<uint32_t>::max();
        if (max_size / sizeof(char_type) < string_length)
        {
            throw_result(xlang_result::invalid_arg, "Insufficient buffer size");
        }
        uint32_t count = sizeof(char_type) * string_length;

        if (max_size - (sizeof(class_type) + sizeof(char_type)) < count)
        {
            throw_result(xlang_result::invalid_arg, "Insufficient buffer size");
        }
        count += (sizeof(class_type) + sizeof(char_type));
        return count;
    }

    template <typename class_type, typename char_type>
    inline char_type* get_packed_buffer_ptr(class_type* ptr)
    {
        return reinterpret_cast<char_type*>(ptr + 1);
    }

    template <typename class_type, typename char_type>
    inline char_type const* get_packed_buffer_ptr(class_type const* ptr)
    {
        return reinterpret_cast<char_type const*>(ptr + 1);
    }
}