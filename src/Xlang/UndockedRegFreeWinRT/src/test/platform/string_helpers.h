#pragma once

// template helpers to genericize test code

template <typename char_type>
auto xlang_create_string(char_type const* source, uint32_t length, xlang_string* str)
{
    static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>);
    if constexpr (std::is_same_v<char_type, xlang_char8>)
    {
        return xlang_create_string_utf8(source, length, str);
    }
    else
    {
        return xlang_create_string_utf16(source, length, str);
    }
}

template <typename char_type>
auto xlang_create_string_reference(char_type const* source, uint32_t length, xlang_string_header* header, xlang_string* str)
{
    static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>);
    if constexpr (std::is_same_v<char_type, xlang_char8>)
    {
        return xlang_create_string_reference_utf8(source, length, header, str);
    }
    else
    {
        return xlang_create_string_reference_utf16(source, length, header, str);
    }
}

template <typename char_type>
auto xlang_get_string_raw_buffer(xlang_string str, char_type const* * buffer, uint32_t* length)
{
    static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>);
    if constexpr (std::is_same_v<char_type, xlang_char8>)
    {
        return xlang_get_string_raw_buffer_utf8(str, buffer, length);
    }
    else
    {
        return xlang_get_string_raw_buffer_utf16(str, buffer, length);
    }
}

template <typename char_type>
auto xlang_preallocate_string_buffer(uint32_t length, char_type** char_buffer, xlang_string_buffer* buffer_handle)
{
    static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>);
    if constexpr (std::is_same_v<char_type, xlang_char8>)
    {
        return xlang_preallocate_string_buffer_utf8(length, char_buffer, buffer_handle);
    }
    else
    {
        return xlang_preallocate_string_buffer_utf16(length, char_buffer, buffer_handle);
    }
}
