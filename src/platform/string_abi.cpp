#include "opaque_string_wrapper.h"
#include "string_reference.h"
#include "pal_error.h"

// Define the ABI-level implementations of string methods

namespace xlang::impl
{
    template <typename char_type>
    struct empty_string_internal;

    template <>
    struct empty_string_internal<xlang_char8>
    {
        static constexpr xlang_char8 value[] = u8"";
    };

    template<>
    struct empty_string_internal<char16_t>
    {
        static constexpr char16_t value[] = u"";
    };

    template <typename char_type>
    xlang_string create_string(char_type const* source_string, uint32_t length)
    {
        if (!source_string && length != 0)
        {
            xlang::throw_result(xlang_result::pointer);
        }

        // length zero is the handle value for an empty string
        if (length != 0)
        {
            return to_handle(heap_string::create(source_string, length));
        }
        return nullptr;
    }

    template <typename char_type>
    xlang_string create_string_reference(
        char_type const* source_string,
        uint32_t length,
        xlang_string_header* header
    )
    {
        if (!source_string && length != 0)
        {
            xlang::throw_result(xlang_result::pointer);
        }

        if (source_string && source_string[length] != 0)
        {
            xlang::throw_result(xlang_result::invalid_arg, "String is not null terminated");
        }

        if (length != 0)
        {
            static_assert(sizeof(string_reference) == sizeof(xlang_string_header), "xlang_string_header must be same size as string_storage_base");
            return to_handle(string_reference::create(source_string, length, reinterpret_cast<string_reference*>(header)));
        }

        return nullptr;
    }

    template <typename char_type>
    uint32_t xlang_get_string_raw_buffer(
        xlang_string string,
        char_type const* * buffer
    )
    {
        if (string)
        {
            auto const result = from_handle(string)->ensure_buffer<char_type>();
            *buffer = result.data();
            return static_cast<uint32_t>(result.size());
        }
        else
        {
            *buffer = empty_string_internal<char_type>::value;
            return 0;
        }
    }

    template <typename char_type>
    xlang_string_buffer preallocate_string_buffer(
        uint32_t length,
        char_type** buffer
    )
    {
        heap_string* result = heap_string::create_preallocated<char_type>(length);
        *buffer = result->mutable_buffer<char_type>();
        return to_buffer_handle(result);
    }
}

using namespace xlang;
using namespace xlang::impl;

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_reference_utf8(
    xlang_char8 const* source_string,
    uint32_t length,
    xlang_string_header* header,
    xlang_string* string
) XLANG_NOEXCEPT
try
{
    *string = xlang::impl::create_string_reference(source_string, length, header);
    return nullptr;
}
catch(...)
{
    *string = nullptr;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_reference_utf16(
    char16_t const* source_string,
    uint32_t length,
    xlang_string_header* header,
    xlang_string* string
) XLANG_NOEXCEPT
try
{
    *string = xlang::impl::create_string_reference(source_string, length, header);
    return nullptr;
}
catch (...)
{
    *string = nullptr;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_utf8(
    xlang_char8 const* source_string,
    uint32_t length,
    xlang_string* string
) XLANG_NOEXCEPT
try
{
    *string = xlang::impl::create_string(source_string, length);
    return nullptr;
}
catch (...)
{
    *string = nullptr;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_create_string_utf16(
    char16_t const* source_string,
    uint32_t length,
    xlang_string* string
) XLANG_NOEXCEPT
try
{
    *string = xlang::impl::create_string(source_string, length);
    return nullptr;
}
catch (...)
{
    *string = nullptr;
    return xlang::to_result();
}

XLANG_PAL_EXPORT void XLANG_CALL xlang_delete_string(xlang_string string) XLANG_NOEXCEPT
{
    string_base* str = from_handle(string);
    if (str)
    {
        str->release_base();
    }
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_delete_string_buffer(xlang_string_buffer buffer_handle) XLANG_NOEXCEPT
try
{
    if (!buffer_handle)
    {
        xlang::throw_result(xlang_result::pointer);
    }

    from_handle(buffer_handle)->free_preallocated();
    return nullptr;
}
catch (...)
{
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_duplicate_string(
    xlang_string string,
    xlang_string* newString
) XLANG_NOEXCEPT
try
{
    *newString = nullptr;

    if (string)
    {
        *newString = to_handle(from_handle(string)->duplicate_base());
    }
    return nullptr;
}
catch (...)
{
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_string_encoding XLANG_CALL xlang_get_string_encoding(
    xlang_string string
) XLANG_NOEXCEPT
{
    if (string)
    {
        bool const is_utf8 = from_handle(string)->is_utf8();
        if (from_handle(string)->has_alternate())
        {
            return static_cast<xlang_string_encoding>(xlang_string_encoding::utf8 | xlang_string_encoding::utf16);
        }
        else
        {
            return is_utf8 ? xlang_string_encoding::utf8 : xlang_string_encoding::utf16;
        }
    }
    else
    {
        return static_cast<xlang_string_encoding>(xlang_string_encoding::utf8 | xlang_string_encoding::utf16);
    }
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_string_raw_buffer_utf8(
    xlang_string string,
    xlang_char8 const* * buffer,
    uint32_t* length
) XLANG_NOEXCEPT
try
{

    *length = xlang::impl::xlang_get_string_raw_buffer(string, buffer);
    return nullptr;
}
catch (...)
{
    *buffer = nullptr;
    *length = 0;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_get_string_raw_buffer_utf16(
    xlang_string string,
    char16_t const* * buffer,
    uint32_t* length
) XLANG_NOEXCEPT
try
{
    *length = xlang::impl::xlang_get_string_raw_buffer(string, buffer);
    return nullptr;
}
catch (...)
{
    *buffer = nullptr;
    *length = 0;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_preallocate_string_buffer_utf8(
    uint32_t length,
    xlang_char8** char_buffer,
    xlang_string_buffer* buffer_handle
) XLANG_NOEXCEPT
try
{
    *buffer_handle = xlang::impl::preallocate_string_buffer(length, char_buffer);
    return nullptr;
}
catch (...)
{
    *char_buffer = nullptr;
    *buffer_handle = nullptr;
    return xlang::to_result();
}


XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_preallocate_string_buffer_utf16(
    uint32_t length,
    char16_t** char_buffer,
    xlang_string_buffer* buffer_handle
) XLANG_NOEXCEPT
try
{
    *buffer_handle = xlang::impl::preallocate_string_buffer(length, char_buffer);
    return nullptr;
}
catch (...)
{
    *char_buffer = nullptr;
    *buffer_handle = nullptr;
    return xlang::to_result();
}

XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_promote_string_buffer(
    xlang_string_buffer buffer_handle,
    xlang_string* string,
    uint32_t length
) XLANG_NOEXCEPT
try
{
    *string = nullptr;

    if (buffer_handle)
    {
        *string = to_handle(from_handle(buffer_handle)->promote_preallocated(length));
    }
    else
    {
        if (length != 0)
        {
            xlang::throw_result(xlang_result::pointer);
        }
    }

    return nullptr;
}
catch (...)
{
    return xlang::to_result();
}
