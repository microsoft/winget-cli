// Temporary win32 placeholder

#include <Windows.h>
#include "pal_internal.h"
#include "string_convert.h"
#include "pal_error.h"

namespace xlang::impl
{
    uint32_t get_converted_length(std::basic_string_view<char16_t> input_str)
    {
        return convert_string(input_str, nullptr, 0);
    }

    uint32_t get_converted_length(std::basic_string_view<xlang_char8> input_str)
    {
        return convert_string(input_str, nullptr, 0);
    }

    uint32_t convert_string(
        std::basic_string_view<char16_t> input_str,
        xlang_char8* output_buffer,
        uint32_t buffer_length)
    {
        static_assert(sizeof(char16_t) == sizeof(wchar_t));
        static_assert(sizeof(xlang_char8) == sizeof(char));
        wchar_t const* input = reinterpret_cast<wchar_t const*>(input_str.data());
        auto const input_length = static_cast<uint32_t>(input_str.size());
        char* output = reinterpret_cast<char*>(output_buffer);

        int result = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input, input_length, output, buffer_length, nullptr, nullptr);
        if (result > 0)
        {
            return static_cast<uint32_t>(result);
        }
        else
        {
            switch (GetLastError())
            {
            case ERROR_INSUFFICIENT_BUFFER:
                throw_result(xlang_result::invalid_arg, "Insufficient buffer size");

            case ERROR_NO_UNICODE_TRANSLATION:
                throw_result(xlang_result::invalid_arg, "Untranslatable string");

            default:
                throw_result(xlang_result::fail);
            }
        }
    }
    
    uint32_t convert_string(
        std::basic_string_view<xlang_char8> input_str,
        char16_t* output_buffer,
        uint32_t buffer_length)
    {
        static_assert(sizeof(char16_t) == sizeof(wchar_t));
        static_assert(sizeof(xlang_char8) == sizeof(char));
        char const* input = reinterpret_cast<char const*>(input_str.data());
        auto const input_length = static_cast<uint32_t>(input_str.size());
        wchar_t* output = reinterpret_cast<wchar_t*>(output_buffer);

        int result = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, input_length, output, buffer_length);
        if (result > 0)
        {
            return static_cast<uint32_t>(result);
        }
        else
        {
            switch (GetLastError())
            {
            case ERROR_INSUFFICIENT_BUFFER:
                throw_result(xlang_result::invalid_arg, "Insufficient buffer size");

            case ERROR_NO_UNICODE_TRANSLATION:
                throw_result(xlang_result::invalid_arg, "Untranslatable string");

            default:
                throw_result(xlang_result::fail);
            }
        }
    }
}