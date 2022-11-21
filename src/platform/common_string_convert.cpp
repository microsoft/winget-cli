#include "pal_internal.h"
#include "string_convert.h"
#include "pal_error.h"
#include "string_traits.h"

namespace xlang::impl::convert
{
    using utf8_worker_t = std::conditional_t<std::is_signed_v<xlang_char8>, uint8_t, xlang_char8>;
    inline auto to_worker(xlang_char8 const* arg) noexcept
    {
        if constexpr (std::is_same_v<xlang_char8, utf8_worker_t>)
        {
            return arg;
        }
        else
        {
            return reinterpret_cast<utf8_worker_t const*>(arg);
        }
    }
    inline auto to_worker(xlang_char8* arg) noexcept
    {
        if constexpr (std::is_same_v<xlang_char8, utf8_worker_t>)
        {
            return arg;
        }
        else
        {
            return reinterpret_cast<utf8_worker_t*>(arg);
        }
    }
    inline char16_t const* to_worker(char16_t const* arg) noexcept { return arg; }
    inline char16_t* to_worker(char16_t* arg) noexcept { return arg; }

    template <typename T>
    struct converter;

    template <>
    struct converter<xlang_char8>
    {
        static char32_t decode(utf8_worker_t const* &begin, utf8_worker_t const* end);
        static void encode(char32_t code_point, utf8_worker_t* &begin, utf8_worker_t* end);
        static uint32_t encoded_length(char32_t code_point);
    };

    template <>
    struct converter<char16_t>
    {
        static char32_t decode(char16_t const* &begin, char16_t const* end);
        static void encode(char32_t code_point, char16_t* &begin, char16_t* end);
        static uint32_t encoded_length(char32_t code_point);
    };

    char32_t converter<xlang_char8>::decode(utf8_worker_t const* &begin, utf8_worker_t const* end)
    {
        char32_t ch = *begin++;
        if (ch <= 0x7f)
        {
            return ch;
        }

        if ((ch & 0xe0) == 0xc0)
        {
            // 2 bytes
            if (end - begin < 1)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            char32_t ch2 = *begin++;
            if ((ch2 & 0xc0) != 0x80)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }

            char32_t result = ((ch & 0x1f) << 6) | (ch2 & 0x3f);
            if (result <= 0x7f)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            return result;
        }
        else if ((ch & 0xf0) == 0xe0)
        {
            if (end - begin < 2)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            char32_t ch2 = *begin++;
            char32_t ch3 = *begin++;
            if ((ch2 & 0xc0) != 0x80 || (ch3 & 0xc0) != 0x80)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            char32_t result = ((ch & 0x0f) << 12) | ((ch2 & 0x3f) << 6) | (ch3 & 0x3f);
            if (result <= 0x7ff || (0xd800 <= result && result <= 0xdfff))
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            return result;
        }
        else if ((ch & 0xf8) == 0xf0)
        {
            if (end - begin < 3)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            char32_t ch2 = *begin++;
            char32_t ch3 = *begin++;
            char32_t ch4 = *begin++;
            if ((ch2 & 0xc0) != 0x80 || (ch3 & 0xc0) != 0x80 || (ch4 & 0xc0) != 0x80)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            char32_t result = ((ch & 0x07) << 18) | ((ch2 & 0x3f) << 12) | ((ch3 & 0x3f) << 6) | (ch4 & 0x3f);
            if (result <= 0xffff || 0x10ffff < result)
            {
                throw_result(xlang_result::invalid_arg, "Untranslatable string");
            }
            return result;
        }
        else
        {
            throw_result(xlang_result::invalid_arg, "Untranslatable string");
        }
    }

    void converter<xlang_char8>::encode(char32_t code_point, utf8_worker_t* &begin, utf8_worker_t* end)
    {
        if (code_point <= 0x7f)
        {
            XLANG_ASSERT(end - begin >= 1);
            *begin++ = static_cast<utf8_worker_t>(code_point);
        }
        else if (code_point <= 0x7ff)
        {
            XLANG_ASSERT(end - begin >= 2);
            *begin++ = static_cast<utf8_worker_t>(0xc0 | (code_point >> 6));
            *begin++ = static_cast<utf8_worker_t>(0x80 | (code_point & 0x3f));
        }
        else if (code_point <= 0xffff)
        {
            XLANG_ASSERT(end - begin >= 3);
            XLANG_ASSERT(code_point < 0xd800 || 0xdfff < code_point);
            *begin++ = static_cast<utf8_worker_t>(0xe0 | (code_point >> 12));
            *begin++ = static_cast<utf8_worker_t>(0x80 | ((code_point >> 6) & 0x3f));
            *begin++ = static_cast<utf8_worker_t>(0x80 | (code_point & 0x3f));
        }
        else
        {
            XLANG_ASSERT(end - begin >= 4);
            XLANG_ASSERT(code_point <= 0x10ffff);
            *begin++ = static_cast<utf8_worker_t>(0xf0 | (code_point >> 18));
            *begin++ = static_cast<utf8_worker_t>(0x80 | ((code_point >> 12) & 0x3f));
            *begin++ = static_cast<utf8_worker_t>(0x80 | ((code_point >> 6) & 0x3f));
            *begin++ = static_cast<utf8_worker_t>(0x80 | (code_point & 0x3f));
        }
    }

    uint32_t converter<xlang_char8>::encoded_length(char32_t code_point)
    {
        if (code_point <= 0x7f)
        {
            return 1;
        }
        else if (code_point <= 0x7ff)
        {
            return 2;
        }
        else if (code_point <= 0xffff)
        {
            XLANG_ASSERT(code_point < 0xd800 || 0xdfff < code_point);
            return 3;
        }
        else
        {
            XLANG_ASSERT(code_point <= 0x10ffff);
            return 4;
        }
    }

    char32_t converter<char16_t>::decode(char16_t const* &begin, char16_t const* end)
    {
        char32_t ch = *begin++;
        if (ch < 0xd800 || 0xdfff < ch)
        {
            return ch;
        }
        else if (ch < 0xdc00 && begin < end)
        {
            char32_t ch2 = *begin++;
            if (0xdc00 <= ch2 && ch2 <= 0xdfff)
            {
                return 0x10000 + ((ch - 0xd800) << 10) | (ch2 - 0xdc00);
            }
        }
        throw_result(xlang_result::invalid_arg, "Untranslatable string");
    }

    void converter<char16_t>::encode(char32_t code_point, char16_t* &begin, char16_t* end)
    {
        if (code_point <= 0xffff)
        {
            XLANG_ASSERT(end - begin >= 1);
            XLANG_ASSERT(code_point < 0xd800 || 0xdfff < code_point);
            *begin++ = static_cast<char16_t>(code_point);
        }
        else
        {
            XLANG_ASSERT(end - begin >= 2);
            XLANG_ASSERT(code_point <= 0x10ffff);
            code_point -= 0x10000;
            *begin++ = static_cast<char16_t>(0xd800 + (code_point >> 10));
            *begin++ = static_cast<char16_t>(0xdc00 + (code_point & 0x3ff));
        }
    }

    uint32_t converter<char16_t>::encoded_length(char32_t code_point)
    {
        if (code_point <= 0xffff)
        {
            XLANG_ASSERT(code_point < 0xd800 || 0xdfff < code_point);
            return 1;
        }
        else
        {
            XLANG_ASSERT(code_point <= 0x10ffff);
            return 2;
        }
    }

    template <typename T>
    uint32_t get_converted_length(std::basic_string_view<T> input_str)
    {
        using output_type = alternate_string_type_t<T>;

        auto input_cursor = to_worker(input_str.data());
        const auto input_end = input_cursor + input_str.size();
        uint32_t length = 0;
        while (input_cursor != input_end)
        {
            auto code_point = converter<T>::decode(input_cursor, input_end);
            length += converter<output_type>::encoded_length(code_point);
        }
        return length;
    }

    template <typename T>
    uint32_t do_conversion(std::basic_string_view<T> input_str, alternate_string_type_t<T> *output_buffer, uint32_t buffer_size)
    {
        using output_type = alternate_string_type_t<T>;

        auto input_cursor = to_worker(input_str.data());
        const auto input_end = input_cursor + input_str.size();

        auto output_cursor = to_worker(output_buffer);
        const auto output_end = output_cursor + buffer_size;
        while (input_cursor != input_end)
        {
            auto code_point = converter<T>::decode(input_cursor, input_end);
            converter<output_type>::encode(code_point, output_cursor, output_end);
        }
        XLANG_ASSERT(output_cursor == output_end);
        return static_cast<uint32_t>(output_cursor - output_end);
    }
}

namespace xlang::impl
{
    uint32_t get_converted_length(std::basic_string_view<char16_t> input_str)
    {
        static_assert(sizeof(xlang_char8) == sizeof(char));
        return convert::get_converted_length(input_str);
    }

    uint32_t get_converted_length(std::basic_string_view<xlang_char8> input_str)
    {
        static_assert(sizeof(xlang_char8) == sizeof(char));
        return convert::get_converted_length(input_str);
    }

    uint32_t convert_string(
        std::basic_string_view<char16_t> input_str,
        xlang_char8* output_buffer,
        uint32_t buffer_size)
    {
        static_assert(sizeof(xlang_char8) == sizeof(char));
        return convert::do_conversion(input_str, output_buffer, buffer_size);
    }
    
    uint32_t convert_string(
        std::basic_string_view<xlang_char8> input_str,
        char16_t* output_buffer,
        uint32_t buffer_size)
    {
        static_assert(sizeof(xlang_char8) == sizeof(char));
        return convert::do_conversion(input_str, output_buffer, buffer_size);
    }
}