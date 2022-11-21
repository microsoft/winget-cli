#pragma once

#include "string_base.h"
#include "atomic_ref_count.h"
#include "heap_string.h"
#include "cache_string.h"

namespace xlang::impl
{
    struct cache_string;

    struct heap_string : string_base
    {
        int32_t addref() noexcept;
        int32_t release() noexcept;

        template <typename char_type>
        static heap_string* create(
            char_type const* source_string,
            uint32_t length);

        template <typename char_type>
        static heap_string* create(
            char_type const* source_string,
            uint32_t length,
            cache_string* alternate);

        template <typename char_type>
        static heap_string* create_preallocated(uint32_t length);

        heap_string* promote_preallocated(uint32_t length);
        void free_preallocated();

        // Read the ptr, but don't create it. May be null.
        cache_string const* get_alternate() const noexcept;
        cache_string* get_alternate() noexcept;

        template <typename char_type>
        char_type* mutable_buffer() noexcept;

        // Diagnostics
        int32_t get_ref_count() const noexcept;
        uint32_t get_total_string_count() const noexcept;

    private:
        template <typename char_type>
        heap_string(
            char_type const* source_string,
            uint32_t length,
            char_type* char_storage
        ) noexcept;

        ~heap_string() noexcept;

        heap_string() = delete;
        heap_string(heap_string const&) = delete;
        heap_string& operator=(heap_string const&) = delete;

        template <typename char_type>
        static heap_string* create_impl(
            char_type const* source_string,
            uint32_t length,
            cache_string* alternate);

        atomic_ref_count count;
        inline static std::atomic<uint32_t> total_string_count{ 0 };
    };

    template <typename char_type>
    heap_string* heap_string::create(
        char_type const* source_string,
        uint32_t length)
    {
        if (length == 0)
        {
            return nullptr;
        }
        return create_impl(source_string, length, nullptr);
    }

    template <typename char_type>
    heap_string* heap_string::create(
        char_type const* source_string,
        uint32_t length,
        cache_string* alternate)
    {
        if (length == 0)
        {
            return nullptr;
        }
        return create_impl(source_string, length, alternate);
    }

    template <typename char_type>
    heap_string* heap_string::create_preallocated(uint32_t length)
    {
        heap_string* result = heap_string::create_impl<char_type>(nullptr, length, nullptr);
        result->mutable_buffer<char_type>()[length] = 0;
        return result;
    }

    inline cache_string const* heap_string::get_alternate() const noexcept
    {
        return this->get_alternate_ptr<cache_string>();
    }

    inline cache_string* heap_string::get_alternate() noexcept
    {
        return this->get_alternate_ptr<cache_string>();
    }

    template <typename char_type>
    inline char_type* heap_string::mutable_buffer() noexcept
    {
        XLANG_ASSERT(is_preallocated_buffer());
        return const_cast<char_type*>(this->get_buffer<char_type>());
    }

    inline int32_t heap_string::get_ref_count() const noexcept
    {
        return count.get_count();
    }

    inline uint32_t heap_string::get_total_string_count() const noexcept
    {
        return total_string_count.load(std::memory_order_acquire);
    }

    template <typename char_type>
    inline heap_string::heap_string(
        char_type const* source_string,
        uint32_t length,
        char_type* char_storage
    ) noexcept
        : string_base(
            char_storage,
            length,
            !source_string ? string_flags::is_preallocated_string_buffer : string_flags::none
        )
    {
        static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>, "char_t must be either xlang_char8 or char16_t");
        // No copy is needed if a caller doesn't want to initialize the buffer.
        // A null source string is used in preallocating string buffers.
        if (source_string)
        {
            std::copy(source_string, source_string + length, char_storage);
        }

        char_storage[length] = 0;

#ifdef _DEBUG
        ++total_string_count;
#endif
    }

    inline heap_string::~heap_string() noexcept
    {
#ifdef _DEBUG
        --total_string_count;
#endif
    }

    inline heap_string* heap_string::promote_preallocated(uint32_t length)
    {
        if (!is_preallocated_buffer())
        {
            throw_result(xlang_result::invalid_arg);
        }
        XLANG_ASSERT(!is_reference());

        if (get_length() < length)
        {
            throw_result(xlang_result::invalid_arg);
        }
        if (length > 0)
        {
            bool const utf8 = is_utf8();
            if (is_utf8())
            {
                mutable_buffer<xlang_char8>()[length] = 0;
            }
            else
            {
                mutable_buffer<char16_t>()[length] = 0;
            }
            update_preallocated_length(length);
            promote_string_buffer_flags();
            XLANG_ASSERT(is_utf8() == utf8);
            XLANG_ASSERT(!is_preallocated_buffer());
            return this;
        }
        else
        {
            release();
            return nullptr;
        }
    }

    inline void heap_string::free_preallocated()
    {
        if (!is_preallocated_buffer())
        {
            throw_result(xlang_result::invalid_arg);
        }
        XLANG_ASSERT(!is_reference());
        release();
    }

    inline int32_t heap_string::addref() noexcept
    {
        return ++count;
    }

    inline int32_t heap_string::release() noexcept
    {
        auto const result = --count;
        if (result == 0)
        {
            auto alternate = get_alternate();
            if (alternate)
            {
                alternate->release();
            }

            xlang_mem_free(this);
        }
        return result;
    }

    template <typename char_type>
    inline heap_string* heap_string::create_impl(
        char_type const* source_string,
        uint32_t length,
        cache_string* alternate)
    {
        heap_string* new_string = reinterpret_cast<heap_string*>(xlang_mem_alloc(packed_buffer_size<heap_string, char_type>(length)));
        if (!new_string)
        {
            throw std::bad_alloc{};
        }

        char_type* buffer = get_packed_buffer_ptr<heap_string, char_type>(new_string);
        new (new_string) heap_string(source_string, length, buffer);

        if (alternate)
        {
            auto new_alternate = new_string->set_alternate_ptr<cache_string>(alternate);
            XLANG_ASSERT(new_alternate == alternate);
            new_alternate->addref();
        }
        return new_string;
    }
}
