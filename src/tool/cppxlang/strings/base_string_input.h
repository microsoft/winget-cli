
namespace xlang::param
{
    struct hstring
    {
        hstring() noexcept : m_handle(nullptr) {}
        hstring(hstring const& values) = delete;
        hstring& operator=(hstring const& values) = delete;
        hstring(std::nullptr_t) = delete;

        ~hstring() noexcept
        {
            xlang_delete_string(m_handle);
        }

        hstring(xlang::hstring const& value) noexcept : m_handle(get_abi(value))
        {
        }

        // char8_t overloads
        hstring(std::basic_string_view<xlang_char8> const& value) noexcept
        {
            init<xlang_char8, false>(value);
        }

        hstring(std::basic_string<xlang_char8> const& value) noexcept
        {
            init<xlang_char8, true>(value);
        }

        hstring(xlang_char8 const* const value) noexcept
        {
            init<xlang_char8, true>(value);
        }

        // char16_t overloads
        hstring(std::basic_string_view<char16_t> const& value) noexcept
        {
            init<char16_t, false>(value);
        }

        hstring(std::basic_string<char16_t> const& value) noexcept
        {
            init<char16_t, true>(value);
        }

        hstring(char16_t const* const value) noexcept
        {
            init<char16_t, true>(value);
        }

#ifdef _WIN32
        // wchar_t overloads
        hstring(std::basic_string_view<wchar_t> const& value) noexcept
        {
            init<wchar_t, false>(value);
        }

        hstring(std::basic_string<wchar_t> const& value) noexcept
        {
            init<wchar_t, true>(value);
        }

        hstring(wchar_t const* const value) noexcept
        {
            init<wchar_t, true>(value);
        }
#endif
        operator xlang::hstring const&() const noexcept
        {
            return *reinterpret_cast<xlang::hstring const*>(this);
        }

    private:
        template <typename char_type, bool is_safe>
        void init(std::basic_string_view<char_type> str) noexcept
        {
            static_assert(impl::is_char_type_supported<char_type>::value);
            auto const value = impl::normalize_char_type(str);
            auto const length = static_cast<uint32_t>(value.size());
            if constexpr (is_safe)
            {
                if constexpr (sizeof(char_type) == sizeof(xlang_char8))
                {
                    XLANG_VERIFY_(nullptr, xlang_create_string_reference_utf8(value.data(), length, &m_header, &m_handle));
                }
                else
                {
                    XLANG_VERIFY_(nullptr, xlang_create_string_reference_utf16(value.data(), length, &m_header, &m_handle));
                }
            }
            else
            {
                if constexpr (sizeof(char_type) == sizeof(xlang_char8))
                {
                    if (nullptr != xlang_create_string_reference_utf8(value.data(), length, &m_header, &m_handle))
                    {
                        std::terminate();
                    }
                }
                else
                {
                    if (nullptr != xlang_create_string_reference_utf16(value.data(), length, &m_header, &m_handle))
                    {
                        std::terminate();
                    }
                }
            }
        }

        xlang_string m_handle;
        xlang_string_header m_header;
    };

    inline xlang_string get_abi(hstring const& object) noexcept
    {
        return *(xlang_string*)(&object);
    }
}

namespace xlang::impl
{
    template <typename T>
    using param_type = std::conditional_t<std::is_same_v<T, hstring>, param::hstring, T>;
}
