// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AppInstaller
{
    // A helper type that resets itself when it is moved from.
    template <typename T>
    struct ResetWhenMovedFrom
    {
        ResetWhenMovedFrom() = default;

        ResetWhenMovedFrom(T t) : m_var(t) {}

        // Not copyable
        ResetWhenMovedFrom(const ResetWhenMovedFrom&) = delete;
        ResetWhenMovedFrom& operator=(const ResetWhenMovedFrom&) = delete;

        ResetWhenMovedFrom(ResetWhenMovedFrom&& other) :
            m_var(std::move(other.m_var))
        {
            other.m_var = T{};
        }

        ResetWhenMovedFrom& operator=(ResetWhenMovedFrom&& other)
        {
            m_var = std::move(other.m_var);
            other.m_var = T{};
            return *this;
        }

        operator T& () { return m_var; }
        operator const T& () const { return m_var; }

    private:
        T m_var;
    };

    // Enables a bool to be used as a destruction indicator.
    using DestructionToken = ResetWhenMovedFrom<bool>;

    // Enable use of folding to execute functions across parameter packs.
    struct FoldHelper
    {
        template <typename T>
        FoldHelper& operator,(T&&) { return *this; }
    };

    // Get the integral value for an enum.
    template <typename E>
    inline std::enable_if_t<std::is_enum_v<E>, std::underlying_type_t<E>> ToIntegral(E e)
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    // Get the enum value for an integral.
    template <typename E>
    inline std::enable_if_t<std::is_enum_v<E>, E> ToEnum(std::underlying_type_t<E> ut)
    {
        return static_cast<E>(ut);
    }
}

// Enable enums to be output generically (as their integral value).
template <typename E>
std::enable_if_t<std::is_enum_v<E>, std::ostream&> operator<<(std::ostream& out, E e)
{
    return out << AppInstaller::ToIntegral(e);
}
