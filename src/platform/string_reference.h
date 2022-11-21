#pragma once

#include "string_base.h"
#include "cache_string.h"

namespace xlang::impl
{
    struct cache_string;

    struct string_reference : string_base
    {
        template <typename char_type>
        static string_reference* create(
            char_type const* source_string,
            uint32_t length,
            string_reference* header
        ) noexcept;

        // Read the ptr, but don't create it. May be null.
        cache_string const* get_alternate() const noexcept;
        cache_string* get_alternate() noexcept;

        void release() noexcept;

    private:
        string_reference() = delete;
        ~string_reference() = delete;
        string_reference(string_reference const&) = delete;
        string_reference& operator=(string_reference const&) = delete;

        // Private ctor that can only be used with placement new
        template <typename char_type>
        string_reference(
            char_type const* source_string,
            uint32_t length
        ) noexcept;
    };

    // string references initialize in the space allocated by xlang_string_header. Size must match.
    static_assert(sizeof(string_reference) == sizeof(xlang_string_header), "size mismatch");
    static_assert(sizeof(string_reference) == sizeof(string_storage_base), "size mismatch");

    template <typename char_type>
    inline string_reference* string_reference::create(
        char_type const* source_string,
        uint32_t length,
        string_reference* header
    ) noexcept
    {
        return (new (header) string_reference{ source_string, length });
    }

    inline cache_string const* string_reference::get_alternate() const noexcept
    {
        return this->get_alternate_ptr<cache_string>();
    }

    inline cache_string* string_reference::get_alternate() noexcept
    {
        return get_alternate_ptr<cache_string>();
    }

    template <typename char_type>
    inline string_reference::string_reference(
        char_type const* source_string,
        uint32_t length
    ) noexcept
        : string_base(source_string, length, string_flags::is_reference)
    {
        static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>, "char_t must be either xlang_char8 or char16_t");
    }

    inline void string_reference::release() noexcept
    {
        auto alternate = get_alternate();
        if (alternate)
        {
            alternate->release();
        }
    }
}