// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

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
}
