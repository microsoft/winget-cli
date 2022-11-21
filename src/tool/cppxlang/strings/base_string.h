
namespace xlang::impl
{
    template <typename char_type>
    struct is_char_type_supported : std::false_type {};

    template <>
    struct is_char_type_supported<xlang_char8> : std::true_type {};

    template <>
    struct is_char_type_supported<char16_t> : std::true_type {};

#ifdef _WIN32
    template <>
    struct is_char_type_supported<wchar_t> : std::true_type {};
#endif

    template <typename char_type>
    auto normalize_char_type(std::basic_string_view<char_type> str) noexcept
    {
        static_assert(is_char_type_supported<char_type>::value);
#ifdef _WIN32
        static_assert(sizeof(wchar_t) == sizeof(char16_t));
        if constexpr (std::is_same_v<char_type, wchar_t>)
        {
            return std::u16string_view{ reinterpret_cast<char16_t const*>(str.data()), str.size() };
        }
        else
#endif
        return str;
    }
    
    inline xlang_string duplicate_string(xlang_string other)
    {
        xlang_string result = nullptr;
        check_xlang_error(xlang_duplicate_string(other, &result));
        return result;
    }

    template <typename char_type, typename = std::enable_if_t<is_char_type_supported<char_type>::value>>
    inline xlang_string create_string(std::basic_string_view<char_type> value)
    {
        xlang_string result = nullptr;
        auto const normalized = normalize_char_type(value);
        auto const length = static_cast<uint32_t>(normalized.size());
        if constexpr (sizeof(char_type) == sizeof(xlang_char8))
        {
            check_xlang_error(xlang_create_string_utf8(normalized.data(), length, &result));
        }
        else
        {
            check_xlang_error(xlang_create_string_utf16(normalized.data(), length, &result));
        }
        return result;
    }

    struct hstring_traits
    {
        using type = xlang_string;

        static void close(type value) noexcept
        {
            xlang_delete_string(value);
        }

        static constexpr type invalid() noexcept
        {
            return nullptr;
        }
    };
}

namespace xlang
{
    struct hstring
    {
        using value_type = xlang_char8;
        using size_type = uint32_t;
        using const_reference = value_type const&;
        using pointer = value_type*;
        using const_pointer = value_type const*;
        using const_iterator = const_pointer;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        hstring() noexcept = default;

        hstring(void* ptr, take_ownership_from_abi_t) noexcept : m_handle(static_cast<xlang_string>(ptr))
        {
        }

        hstring(hstring const& value) :
            m_handle(impl::duplicate_string(value.m_handle.get()))
        {}

        hstring& operator=(hstring const& value)
        {
            m_handle.attach(impl::duplicate_string(value.m_handle.get()));
            return*this;
        }

        hstring(hstring&&) noexcept = default;
        hstring& operator=(hstring&&) = default;
        hstring(std::nullptr_t) = delete;

        // char8_t overloads
        hstring(std::initializer_list<xlang_char8> value) :
            hstring(std::basic_string_view<xlang_char8>(value.begin(), value.size()))
        {}

        hstring(xlang_char8 const* value) :
            hstring(std::basic_string_view<xlang_char8>(value))
        {}

        hstring(xlang_char8 const* value, size_type size) :
            hstring(std::basic_string_view<xlang_char8>(value, size))
        {}

        explicit hstring(std::basic_string_view<xlang_char8> const& value) :
            m_handle(impl::create_string(value))
        {}

        hstring& operator=(std::basic_string_view<xlang_char8> const& value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(xlang_char8 const* const value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(std::initializer_list<xlang_char8> value)
        {
            return *this = hstring{ value };
        }

        // char16_t overloads
        hstring(std::initializer_list<char16_t> value) :
            hstring(std::basic_string_view<char16_t>(value.begin(), value.size()))
        {}

        hstring(char16_t const* value) :
            hstring(std::basic_string_view<char16_t>(value))
        {}

        hstring(char16_t const* value, size_type size) :
            hstring(std::basic_string_view<char16_t>(value, size))
        {}

        explicit hstring(std::basic_string_view<char16_t> const& value) :
            m_handle(impl::create_string(value))
        {}

        hstring& operator=(std::basic_string_view<char16_t> const& value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(char16_t const* const value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(std::initializer_list<char16_t> value)
        {
            return *this = hstring{ value };
        }

        // wchar_t overloads, just for Windows
#ifdef _WIN32
        hstring(std::initializer_list<wchar_t> value) :
            hstring(std::basic_string_view<wchar_t>(value.begin(), value.size()))
        {}

        hstring(wchar_t const* value) :
            hstring(std::basic_string_view<wchar_t>(value))
        {}

        hstring(wchar_t const* value, size_type size) :
            hstring(std::basic_string_view<wchar_t>(value, size))
        {}

        explicit hstring(std::basic_string_view<wchar_t> const& value) :
            m_handle(impl::create_string(value))
        {}

        hstring& operator=(std::basic_string_view<wchar_t> const& value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(wchar_t const* const value)
        {
            return *this = hstring{ value };
        }

        hstring& operator=(std::initializer_list<wchar_t> value)
        {
            return *this = hstring{ value };
        }
#endif

        void clear() noexcept
        {
            m_handle.close();
        }

        operator std::basic_string_view<xlang_char8>() const noexcept
        {
            uint32_t size;
            xlang_char8 const* data;
            check_xlang_error(xlang_get_string_raw_buffer_utf8(m_handle.get(), &data, &size));
            return { data, size };
        }

        const_reference operator[](size_type pos) const noexcept
        {
            XLANG_ASSERT(pos < size());
            return*(begin() + pos);
        }

        const_reference front() const noexcept
        {
            XLANG_ASSERT(!empty());
            return*begin();
        }

        const_reference back() const noexcept
        {
            XLANG_ASSERT(!empty());
            return*(end() - 1);
        }

        const_pointer data() const noexcept
        {
            return begin();
        }

        const_pointer c_str() const noexcept
        {
            return begin();
        }

        const_iterator begin() const noexcept
        {
            return std::basic_string_view<value_type>(*this).data();
        }

        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        const_iterator end() const noexcept
        {
            auto view = std::basic_string_view<value_type>(*this);
            return view.data() + view.size();
        }

        const_iterator cend() const noexcept
        {
            return end();
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        bool empty() const noexcept
        {
            return !m_handle;
        }

        size_type size() const noexcept
        {
            return static_cast<size_type>(std::basic_string_view<value_type>(*this).size());
        }

        friend void swap(hstring& left, hstring& right) noexcept
        {
            swap(left.m_handle, right.m_handle);
        }

    private:
        handle_type<impl::hstring_traits> m_handle;
    };

    inline xlang_string get_abi(hstring const& object) noexcept
    {
        return *(xlang_string*)(&object);
    }

    inline xlang_string* put_abi(hstring& object) noexcept
    {
        object.clear();
        return reinterpret_cast<xlang_string*>(&object);
    }

    inline void attach_abi(hstring& object, xlang_string value) noexcept
    {
        object.clear();
        *put_abi(object) = value;
    }

    inline xlang_string detach_abi(hstring& object) noexcept
    {
        xlang_string temp = get_abi(object);
        *reinterpret_cast<xlang_string*>(&object) = nullptr;
        return temp;
    }

    inline xlang_string detach_abi(hstring&& object) noexcept
    {
        return detach_abi(object);
    }

    inline void copy_from_abi(hstring& object, xlang_string value)
    {
        attach_abi(object, impl::duplicate_string(value));
    }

    inline void copy_to_abi(hstring const& object, xlang_string& value)
    {
        XLANG_ASSERT(value == nullptr);
        value = impl::duplicate_string(get_abi(object));
    }

    inline xlang_string detach_abi(std::basic_string_view<xlang_char8> const& value)
    {
        return impl::create_string(value);
    }

    inline xlang_string detach_abi(xlang_char8 const* const value)
    {
        return impl::create_string(std::basic_string_view<xlang_char8>(value));
    }

    inline xlang_string detach_abi(std::basic_string_view<char16_t> const& value)
    {
        return impl::create_string(value);
    }

    inline xlang_string detach_abi(char16_t const* const value)
    {
        return impl::create_string(std::basic_string_view<char16_t>(value));
    }

#ifdef _WIN32
    inline xlang_string detach_abi(std::basic_string_view<wchar_t> const& value)
    {
        return impl::create_string(value);
    }

    inline xlang_string detach_abi(wchar_t const* const value)
    {
        return impl::create_string(std::basic_string_view<wchar_t>(value));
    }
#endif
}

namespace xlang::impl
{
    template <> struct abi<hstring>
    {
        using type = xlang_string;
    };

    template <> struct name<hstring>
    {
        static constexpr auto & value{ u8"String" };
        static constexpr auto & data{ u8"string" };
    };

    template <> struct category<hstring>
    {
        using type = basic_category;
    };

    struct hstring_builder
    {
        hstring_builder(hstring_builder const&) = delete;
        hstring_builder& operator=(hstring_builder const&) = delete;

        explicit hstring_builder(uint32_t const size)
            : m_size(size)
        {
            check_xlang_error(xlang_preallocate_string_buffer_utf8(m_size, &m_data, &m_buffer));
        }

        ~hstring_builder() noexcept
        {
            if (m_buffer != nullptr)
            {
                check_xlang_error(xlang_delete_string_buffer(m_buffer));
            }
        }

        xlang_char8* data() noexcept
        {
            XLANG_ASSERT(m_buffer != nullptr);
            return m_data;
        }

        hstring to_hstring()
        {
            XLANG_ASSERT(m_buffer != nullptr);
            xlang_string result;
            check_xlang_error(xlang_promote_string_buffer(m_buffer, &result, m_size));
            m_buffer = nullptr;
            return { result, take_ownership_from_abi };
        }

        void size(uint32_t value) noexcept
        {
            XLANG_ASSERT(value <= m_original_size);
            m_size = value;
        }

    private:

        xlang_char8* m_data{ nullptr };
        xlang_string_buffer m_buffer{ nullptr };
        uint32_t m_size{};
        uint32_t const m_original_size{ m_size };
    };
}

namespace xlang
{
    inline hstring to_hstring(uint8_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRIu8, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(int8_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRId8, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(uint16_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRIu16, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(int16_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRId16, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(uint32_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRIu32, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(int32_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRId32, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(uint64_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRIu64, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(int64_t value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%" PRId64, value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(float value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%G", value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(double value)
    {
        char buffer[32];
        snprintf(buffer, std::size(buffer), "%G", value);
        return hstring{ buffer };
    }

    inline hstring to_hstring(char16_t value)
    {
        char16_t buffer[2] = { value, 0 };
        return hstring{ buffer };
    }

    inline hstring to_hstring(hstring const& value) noexcept
    {
        return value;
    }

    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    hstring to_hstring(T const value)
    {
        if (value)
        {
            return hstring{ "true" };
        }
        else
        {
            return hstring{"false" };
        }
    }

    inline hstring to_hstring(guid const& value)
    {
        char buffer[40];
        //{00000000-0000-0000-0000-000000000000}
        snprintf(buffer, std::size(buffer), "{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}",
            value.Data1, value.Data2, value.Data3, value.Data4[0], value.Data4[1],
            value.Data4[2], value.Data4[3], value.Data4[4], value.Data4[5], value.Data4[6], value.Data4[7]);
        return hstring{ buffer };
    }

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, std::string_view>>>
    hstring to_hstring(T const& value)
    {
        std::string_view const view(value);
        return hstring{ view };
    }
}
