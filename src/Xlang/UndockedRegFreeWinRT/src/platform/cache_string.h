#pragma once

#include <memory>
#include "pal_internal.h"
#include "atomic_ref_count.h"
#include "string_allocate.h"
#include "string_convert.h"
#include "string_traits.h"

namespace xlang::impl
{
    struct xlang_mem_deleter
    {
        void operator()(void* ptr) const noexcept
        {
            xlang_mem_free(ptr);
        }
    };

    struct cache_string
    {
        template <typename char_type>
        static std::unique_ptr<cache_string, xlang_mem_deleter> create(char_type const* source_string, uint32_t length);

        template <typename char_type>
        char_type const* get_buffer() const noexcept;

        void addref() noexcept;
        void release() noexcept;

        uint32_t get_length() const noexcept;

    private:
        explicit cache_string(uint32_t length)
            : length_(length)
        {}

        cache_string() = delete;
        atomic_ref_count count;
        uint32_t length_{};
    };

    inline void cache_string::addref() noexcept
    {
        ++count;
    }

    inline void cache_string::release() noexcept
    {
        if (--count == 0)
        {
            xlang_mem_free(this);
        }
    }

    template <typename char_type>
    std::unique_ptr<cache_string, xlang_mem_deleter> cache_string::create(char_type const* source_string, uint32_t length)
    {
        static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>, "char_t must be either xlang_char8 or char16_t");
        using alternate_char_type = alternate_string_type_t<char_type>;
        uint32_t alternate_length = get_converted_length({ source_string, length });

        auto packed_size = packed_buffer_size<cache_string, alternate_char_type>(alternate_length);

        std::unique_ptr<cache_string, xlang_mem_deleter> new_string{ reinterpret_cast<cache_string*>(xlang_mem_alloc(packed_size)) };
        if (!new_string)
        {
            throw std::bad_alloc{};
        }

        alternate_char_type* alternate_buffer = get_packed_buffer_ptr<cache_string, alternate_char_type>(new_string.get());
        convert_string({ source_string, length }, alternate_buffer, alternate_length);
        alternate_buffer[alternate_length] = 0;

        new (new_string.get()) cache_string(alternate_length);
        return new_string;
    }

    template <typename char_type>
    inline char_type const* cache_string::get_buffer() const noexcept
    {
        return get_packed_buffer_ptr<cache_string, char_type>(this);
    }

    inline uint32_t cache_string::get_length() const noexcept
    {
        return length_;
    }
}
