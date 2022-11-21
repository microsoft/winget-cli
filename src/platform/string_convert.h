#pragma once

#include "pal.h"
#include <stdint.h>
#include <optional>
#include <string_view>

namespace xlang::impl
{
    uint32_t get_converted_length(std::basic_string_view<char16_t> input_str);
    uint32_t get_converted_length(std::basic_string_view<xlang_char8> input_str);

    uint32_t convert_string(
        std::basic_string_view<char16_t> input_str,
        xlang_char8* output_buffer,
        uint32_t buffer_size);

    uint32_t convert_string(
        std::basic_string_view<xlang_char8> input_str,
        char16_t* output_buffer,
        uint32_t buffer_size);
}