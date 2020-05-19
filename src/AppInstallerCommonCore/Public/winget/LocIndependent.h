// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <string_view>

namespace AppInstaller::Utility
{
    // "I solemnly swear that this string is indeed localization independent."
    // A localization independent string view.
    // Used as a wrapper around strings that do not need localization.
    struct LocIndView : public std::string_view
    {
        explicit constexpr LocIndView(std::string_view sv) : std::string_view(sv) {}
    };

    namespace literals
    {
        // "I solemnly swear that this string is indeed localization independent."
        // Enable easier use of a localization independent view through literals.
        inline LocIndView operator ""_liv(const char* chars, size_t size)
        {
            return LocIndView{ std::string_view{ chars, size } };
        }
    }

    // "I solemnly swear that this string is indeed localization independent."
    // A localization independent string; either through external localization
    // or by virtue of not needing to be localized.
    // Intentionally does not allow the value to be modified to prevent accidental
    // reintroduction of a localization dependent value.
    struct LocIndString
    {
        LocIndString() = default;

        explicit LocIndString(std::string v) : m_value(std::move(v)) {}

        LocIndString(const LocIndString&) = default;
        LocIndString& operator=(const LocIndString&) = default;

        LocIndString(LocIndString&&) = default;
        LocIndString& operator=(LocIndString&&) = default;

        const std::string& get() const { return m_value; }

        operator const std::string& () const { return m_value; }
        operator std::string_view() const { return m_value; }

        const std::string* operator->() const { return &m_value; }

        bool operator==(std::string_view sv) { return m_value == sv; }

    private:
        std::string m_value;
    };
}

inline std::ostream& operator<<(std::ostream& out, const AppInstaller::Utility::LocIndString& lis)
{
    return (out << lis.get());
}
