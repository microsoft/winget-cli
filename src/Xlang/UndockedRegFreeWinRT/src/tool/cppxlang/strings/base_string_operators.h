
namespace xlang
{
    inline bool operator==(hstring const& left, hstring const& right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) == std::basic_string_view<xlang_char8>(right);
    }

    inline bool operator==(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) == right;
    }

    inline bool operator==(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept
    {
        return left == std::basic_string_view<xlang_char8>(right);
    }

    inline bool operator==(hstring const& left, xlang_char8 const* right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) == right;
    }

    inline bool operator==(xlang_char8 const* left, hstring const& right) noexcept
    {
        return left == std::basic_string_view<xlang_char8>(right);
    }

    bool operator==(hstring const& left, std::nullptr_t) = delete;

    bool operator==(std::nullptr_t, hstring const& right) = delete;

    inline bool operator<(hstring const& left, hstring const& right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) < std::basic_string_view<xlang_char8>(right);
    }

    inline bool operator<(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept
    {
        return left < std::basic_string_view<xlang_char8>(right);
    }

    inline bool operator<(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) < right;
    }

    inline bool operator<(hstring const& left, xlang_char8 const* right) noexcept
    {
        return std::basic_string_view<xlang_char8>(left) < right;
    }

    inline bool operator<(xlang_char8 const* left, hstring const& right) noexcept
    {
        return left < std::basic_string_view<xlang_char8>(right);
    }

    bool operator<(hstring const& left, std::nullptr_t) = delete;

    bool operator<(std::nullptr_t, hstring const& right) = delete;
    inline bool operator!=(hstring const& left, hstring const& right) noexcept { return !(left == right); }
    inline bool operator>(hstring const& left, hstring const& right) noexcept { return right < left; }
    inline bool operator<=(hstring const& left, hstring const& right) noexcept { return !(right < left); }
    inline bool operator>=(hstring const& left, hstring const& right) noexcept { return !(left < right); }

    inline bool operator!=(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept { return !(left == right); }
    inline bool operator>(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept { return right < left; }
    inline bool operator<=(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept { return !(right < left); }
    inline bool operator>=(hstring const& left, std::basic_string<xlang_char8> const& right) noexcept { return !(left < right); }

    inline bool operator!=(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept { return !(left == right); }
    inline bool operator>(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept { return right < left; }
    inline bool operator<=(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept { return !(right < left); }
    inline bool operator>=(std::basic_string<xlang_char8> const& left, hstring const& right) noexcept { return !(left < right); }

    inline bool operator!=(hstring const& left, xlang_char8 const* right) noexcept { return !(left == right); }
    inline bool operator>(hstring const& left, xlang_char8 const* right) noexcept { return right < left; }
    inline bool operator<=(hstring const& left, xlang_char8 const* right) noexcept { return !(right < left); }
    inline bool operator>=(hstring const& left, xlang_char8 const* right) noexcept { return !(left < right); }

    inline bool operator!=(xlang_char8 const* left, hstring const& right) noexcept { return !(left == right); }
    inline bool operator>(xlang_char8 const* left, hstring const& right) noexcept { return right < left; }
    inline bool operator<=(xlang_char8 const* left, hstring const& right) noexcept { return !(right < left); }
    inline bool operator>=(xlang_char8 const* left, hstring const& right) noexcept { return !(left < right); }

    bool operator!=(hstring const& left, std::nullptr_t right) = delete;
    bool operator>(hstring const& left, std::nullptr_t right) = delete;
    bool operator<=(hstring const& left, std::nullptr_t right) = delete;
    bool operator>=(hstring const& left, std::nullptr_t right) = delete;

    bool operator!=(std::nullptr_t left, hstring const& right) = delete;
    bool operator>(std::nullptr_t left, hstring const& right) = delete;
    bool operator<=(std::nullptr_t left, hstring const& right) = delete;
    bool operator>=(std::nullptr_t left, hstring const& right) = delete;
}

namespace xlang::impl
{
    inline hstring concat_hstring(std::basic_string_view<xlang_char8> const& left, std::basic_string_view<xlang_char8> const& right)
    {
        hstring_builder text(static_cast<uint32_t>(left.size() + right.size()));
        auto cursor = std::copy_n(left.data(), left.size(), text.data());
        std::copy_n(right.data(), right.size(), cursor);
        return text.to_hstring();
    }
}

namespace xlang
{
    inline hstring operator+(hstring const& left, hstring const& right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(hstring const& left, std::basic_string<xlang_char8> const& right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(std::basic_string<xlang_char8> const& left, hstring const& right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(hstring const& left, xlang_char8 const* right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(xlang_char8 const* left, hstring const& right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(hstring const& left, xlang_char8 right)
    {
        return impl::concat_hstring(left, std::basic_string_view<xlang_char8>(&right, 1));
    }

    inline hstring operator+(xlang_char8 left, hstring const& right)
    {
        return impl::concat_hstring(std::basic_string_view<xlang_char8>(&left, 1), right);
    }

    hstring operator+(hstring const& left, std::nullptr_t) = delete;

    hstring operator+(std::nullptr_t, hstring const& right) = delete;

    inline hstring operator+(hstring const& left, std::basic_string_view<xlang_char8> const& right)
    {
        return impl::concat_hstring(left, right);
    }

    inline hstring operator+(std::basic_string_view<xlang_char8> const& left, hstring const& right)
    {
        return impl::concat_hstring(left, right);
    }
}
