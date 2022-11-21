#pragma once

#include <atomic>
#include <type_traits>
#include <algorithm>
#include "pal_internal.h"
#include "cache_string.h"
#include "string_traits.h"

namespace xlang::impl
{
    // Details of the fundamental data storage underpinning xlang_string
    enum class string_flags : uint32_t
    {
        none = 0x0000,         // None
        is_reference = 0x0001, // Whether this is a "fast" string
        is_utf8 = 0x0020,      // Character pointer is UTF-8 data

        is_preallocated_string_buffer = 0xF8B10000,
        reserved_for_preallocated_string_buffer = 0xFFFF0000, // Reserved bits that are set to a specifc value if this is a preallocated string buffer.
                                                              // Ensures that the magic value above cannot be coincidentally shared by some
                                                              // valid future combination of string_flags
        flags_to_preserve_during_promote = is_utf8,           // Some flags should be preserved after promoting a preallocated buffer.
    };

    constexpr string_flags operator | (string_flags lhs, string_flags rhs) noexcept
    {
        using int_t = std::underlying_type_t<string_flags>;
        return static_cast<string_flags>(static_cast<int_t>(lhs) | static_cast<int_t>(rhs));
    }

    constexpr string_flags& operator |= (string_flags& lhs, string_flags rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr string_flags operator & (string_flags lhs, string_flags rhs) noexcept
    {
        using int_t = std::underlying_type_t<string_flags>;
        return static_cast<string_flags>(static_cast<int_t>(lhs) & static_cast<int_t>(rhs));
    }

    constexpr string_flags& operator &= (string_flags& lhs, string_flags rhs) noexcept
    {
        lhs = lhs & rhs;
        return lhs;
    }

    constexpr string_flags operator~(string_flags arg) noexcept
    {
        using int_t = std::underlying_type_t<string_flags>;
        return static_cast<string_flags>(~static_cast<int_t>(arg));
    }

    inline constexpr string_flags all_valid_flags =
        string_flags::is_reference |
        string_flags::is_utf8 |
        string_flags::reserved_for_preallocated_string_buffer;

    struct string_storage_base
    {
        // The only time flags and length can change is while promoting a preallocated buffer.
        // Preallocated buffers shouldn't be copied, so only a single thread should ever promote it.
        // Therefore, these two fields don't need to be synchronized.
        string_flags flags{};
        uint32_t length_{};
        struct padding_struct
        {
            uint32_t padding1;
            uint32_t padding2;
        };
        union
        {
            padding_struct padding;
            std::atomic<void*> alternate_form{ nullptr }; // If string_flags::is_alternate is set, this contains a pointer to another representation of a string
            static_assert(sizeof(alternate_form) <= sizeof(padding), "Padding overrun");
        };
        union
        {
            // Points to a UTF-16 or UTF-8 string buffer
            void const* string_ref{ nullptr };
        };
    };

    // String objects are represented by a set of classes that all derive from string_base. The
    // data for string_base (the common string header data) is defined separately in the
    // string_storage_base struct for debugging and testing purposes. Aside from those cases, all
    // string handling is done through these class implementations. Since each type has its own special
    // memory management, constructors are hidden or deleted, and instances are created though
    // 'typename::Create' methods.
    //
    // The sub-classes represent the various types of strings that can exist.
    //      string_reference is a non-shared string representation initialized into
    //          a buffer provided by the caller.
    //
    //      heap_string is a shared, immutable, heap-allocated string instance that packes the
    //          string header data and character data into a single allocation.
    //
    // cache_string holds is *NOT* a sub-class of string_base.
    //     It holds string buffer data when a raw buffer is requested in a different
    //     encoding than that of the original string_rerefence/heap_string

    struct string_base : private string_storage_base
    {
        string_base* duplicate_base();
        void release_base() noexcept;

        uint32_t get_length() const noexcept;

        template <typename char_type>
        std::basic_string_view<char_type> ensure_buffer();

        template <typename char_type>
        char_type const* get_buffer() const noexcept;

        bool is_reference() const noexcept;
        bool is_preallocated_buffer() const noexcept;
        bool is_utf8() const noexcept;
        bool has_alternate() const noexcept;

    protected:
        string_base() = delete;
        string_base(string_base const&) = delete;
        string_base& operator=(string_base const&) = delete;

        ~string_base() = default;

        void update_preallocated_length(uint32_t length) noexcept;

        template <typename char_type>
        string_base(char_type const* storage, uint32_t length, string_flags new_flags = string_flags::none) noexcept;

        void promote_string_buffer_flags() noexcept;

        // Get or set the alternate representation string, in a thread-safe manner
        template <typename alternate_type>
        alternate_type const* get_alternate_ptr() const noexcept;

        template <typename alternate_type>
        alternate_type* get_alternate_ptr() noexcept;

        template <typename alternate_type>
        alternate_type* set_alternate_ptr(alternate_type* new_alternate) noexcept;

    private:
        template <typename my_char_type, typename requested_char_type>
        std::basic_string_view<requested_char_type> ensure_buffer_impl();
    };

    // This class is a wrapper, need to be able to up-cast safely, which means layout can't change.
    static_assert(sizeof(string_base) == sizeof(string_storage_base), "Class layout must match");

    inline uint32_t string_base::get_length() const noexcept
    {
        return this->length_;
    }

    template <typename char_type>
    inline char_type const* string_base::get_buffer() const noexcept
    {
        return reinterpret_cast<char_type const*>(this->string_ref);
    }

    inline bool string_base::is_reference() const noexcept
    {
        return (flags & string_flags::is_reference) != string_flags::none;
    }

    inline bool string_base::is_preallocated_buffer() const noexcept
    {
        return (flags & string_flags::reserved_for_preallocated_string_buffer) == string_flags::is_preallocated_string_buffer;
    }

    inline bool string_base::is_utf8() const noexcept
    {
        return (flags & string_flags::is_utf8) != string_flags::none;
    }

    inline bool string_base::has_alternate() const noexcept
    {
        return get_alternate_ptr<cache_string>();
    }

    template <typename char_type>
    inline string_base::string_base(char_type const* storage, uint32_t length, string_flags new_flags) noexcept
        : string_storage_base{}
    {
        static_assert(std::disjunction_v<std::is_same<char_type, xlang_char8>, std::is_same<char_type, char16_t>>, "char_t must be either xlang_char8 or char16_t");
        constexpr string_flags is_utf8{ std::is_same_v<char_type, xlang_char8> ? string_flags::is_utf8 : string_flags::none };
        this->flags = new_flags | is_utf8;
        this->string_ref = storage;
        this->length_ = length;
    }

    inline void string_base::update_preallocated_length(uint32_t length) noexcept
    {
        this->length_ = length;
    }

    inline void string_base::promote_string_buffer_flags() noexcept
    {
        auto const preserved = flags & string_flags::flags_to_preserve_during_promote;
        flags = preserved;
    }

    template <typename alternate_type>
    inline alternate_type const* string_base::get_alternate_ptr() const noexcept
    {
        return reinterpret_cast<alternate_type const*>(this->alternate_form.load());
    }

    template <typename alternate_type>
    inline alternate_type* string_base::get_alternate_ptr() noexcept
    {
        return reinterpret_cast<alternate_type*>(this->alternate_form.load());
    }

    template <typename alternate_type>
    inline alternate_type* string_base::set_alternate_ptr(alternate_type* new_alternate) noexcept
    {
        // This function is write-once. There's the potential to race on setting alternate. This function returns
        /// the value of the alternate actually set, which may be different from the value provided. In that case,
        // the caller is responsible for cleaning up the value that was not set.
        void* expected = nullptr;
        bool result = alternate_form.compare_exchange_strong(expected, new_alternate);

        // result has the old value now. If it was null, we set the new one. Otherwise result is still the current
        // alternate.
        return result ? new_alternate : reinterpret_cast<alternate_type*>(expected);
    }

    template <typename char_type>
    inline std::basic_string_view<char_type> string_base::ensure_buffer()
    {
        if (is_utf8())
        {
            return ensure_buffer_impl<xlang_char8, char_type>();
        }
        else
        {
            return ensure_buffer_impl<char16_t, char_type>();
        }
    }

    template <typename my_char_type, typename requested_char_type>
    inline std::basic_string_view<requested_char_type> string_base::ensure_buffer_impl()
    {
        if constexpr (std::is_same_v<my_char_type, requested_char_type>)
        {
            return { get_buffer<my_char_type>(), get_length() };
        }
        else
        {
            cache_string* alternate = get_alternate_ptr<cache_string>();
            if (!alternate)
            {
                auto new_alternate = cache_string::create(get_buffer<my_char_type>(), get_length());
                alternate = set_alternate_ptr<cache_string>(new_alternate.get());
                if (alternate == new_alternate.get())
                {
                    // We won the race. Can safely handoff lifetime management to string_base
                    new_alternate.release();
                }
            }
            return { alternate->get_buffer<requested_char_type>(), alternate->get_length() };
        }
    }
}
