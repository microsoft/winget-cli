#pragma once

#include <iterator>
#include <string_view>

#include "impl/base.h"

struct namespace_iterator
{
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::string_view;
    using reference = value_type;

    // Proxy type for operator->
    struct pointer
    {
        std::string_view value;
        std::string_view* operator->() { return &value; }
    };

    constexpr namespace_iterator(std::string_view ns, std::string_view::size_type index) noexcept :
        m_fullNamespace(ns),
        m_index(index)
    {
        if (m_index != std::string_view::npos)
        {
            initialize_current();
        }
    }

    constexpr reference operator*() const noexcept
    {
        XLANG_ASSERT(valid());
        return m_currentNamespace;
    }

    constexpr pointer operator->() const noexcept
    {
        XLANG_ASSERT(valid());
        return pointer{ m_currentNamespace };
    }

    constexpr namespace_iterator& operator++() noexcept
    {
        advance_forward();
        return *this;
    }

    constexpr namespace_iterator operator++(int) noexcept
    {
        auto copy = *this;
        advance_forward();
        return copy;
    }

    constexpr namespace_iterator& operator--() noexcept
    {
        advance_backward();
        return *this;
    }

    constexpr namespace_iterator operator--(int) noexcept
    {
        auto copy = *this;
        advance_backward();
        return copy;
    }

    friend constexpr bool operator==(const namespace_iterator& lhs, const namespace_iterator& rhs) noexcept
    {
        assert(lhs.m_fullNamespace.data() == rhs.m_fullNamespace.data());
        return lhs.m_index == rhs.m_index;
    }

    friend constexpr bool operator!=(const namespace_iterator& lhs, const namespace_iterator& rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:

    constexpr bool valid() const noexcept
    {
        return m_index != std::string_view::npos;
    }

    constexpr void initialize_current() noexcept
    {
        XLANG_ASSERT(valid() && (m_index < m_fullNamespace.length()));

        auto pos = m_fullNamespace.find_first_of('.', m_index);
        auto length = pos - m_index;
        XLANG_ASSERT(length > 0);

        m_currentNamespace = m_fullNamespace.substr(m_index, length);
    }

    constexpr void advance_forward() noexcept
    {
        XLANG_ASSERT(valid());
        m_index += m_currentNamespace.length();
        if (m_index >= m_fullNamespace.length())
        {
            m_index = std::string_view::npos;
            m_currentNamespace = {};
        }
        else
        {
            ++m_index; // Move past the '.'
            initialize_current();
        }
    }

    constexpr void advance_backward() noexcept
    {
        XLANG_ASSERT(m_index != 0);

        auto endPos = m_index;
        auto pos = m_fullNamespace.find_last_of('.', m_index - 2);
        if (pos == std::string_view::npos)
        {
            // We've hit the beginning
            m_index = 0;
        }
        else
        {
            m_index = pos + 1; // 'pos' is the index of the period
        }

        m_currentNamespace = m_fullNamespace.substr(m_index, endPos - m_index - 1);
    }

    std::string_view m_fullNamespace;
    std::string_view m_currentNamespace;
    std::string_view::size_type m_index;
};

struct namespace_range
{
    using const_iterator = namespace_iterator;
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;

    std::string_view ns;

    constexpr const_iterator begin() const noexcept
    {
        return const_iterator{ ns, 0 };
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    constexpr const_iterator end() const noexcept
    {
        return const_iterator{ ns, std::string_view::npos };
    }

    constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator{ end() };
    }

    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator{ begin() };
    }

    constexpr const_reverse_iterator crend() const noexcept
    {
        return rend();
    }
};

struct reverse_namespace_range
{
    using const_iterator = std::reverse_iterator<namespace_iterator>;
    using iterator = const_iterator;

    std::string_view ns;

    constexpr const_iterator begin() const noexcept
    {
        return const_iterator{ namespace_iterator{ ns, std::string_view::npos } };
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    constexpr const_iterator end() const noexcept
    {
        return const_iterator{ namespace_iterator{ ns, 0 } };
    }

    constexpr const_iterator cend() const noexcept
    {
        return end();
    }
};
