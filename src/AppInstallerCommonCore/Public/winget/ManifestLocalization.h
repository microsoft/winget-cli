// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>

#include <variant>

namespace AppInstaller::Manifest
{
    using string_t = Utility::NormalizedString;

    enum class Localization : size_t
    {
        Publisher,
        PublisherUrl,
        PublisherSupportUrl,
        PrivacyUrl,
        Author,
        PackageName,
        PackageUrl,
        License,
        LicenseUrl,
        Copyright,
        CopyrightUrl,
        ShortDescription,
        Description,
        Tags,
        Max
    };

    namespace details
    {
        template <Localization L>
        struct LocalizationMapping
        {
            // value_t type specifies the type of this data
        };

        template <>
        struct LocalizationMapping<Localization::Publisher>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::PublisherUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::PublisherSupportUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::PrivacyUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::Author>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::PackageName>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::PackageUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::License>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::LicenseUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::Copyright>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::CopyrightUrl>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::ShortDescription>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::Description>
        {
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::Tags>
        {
            using value_t = std::vector<string_t>;
        };

        // Used to deduce the DataVariant type; making a variant that includes std::monostate and all DataMapping types.
        template <size_t... I>
        inline auto Deduce(std::index_sequence<I...>) { return std::variant<std::monostate, LocalizationMapping<static_cast<Localization>(I)>::value_t...>{}; }

        // Holds data of any type listed in a DataMapping.
        using LocalizationVariant = decltype(Deduce(std::make_index_sequence<static_cast<size_t>(Localization::Max)>()));

        // Gets the index into the variant for the given Data.
        constexpr inline size_t LocalizationIndex(Localization l) { return static_cast<size_t>(l) + 1; }
    }

    struct ManifestLocalization
    {
        string_t Locale;

        // Adds a value to the Localization data, or overwrites an existing entry.
        // This must be used to create the initial entry, but Get can be used to modify.
        template <Localization L>
        void Add(typename details::LocalizationMapping<L>::value_t&& v)
        {
            m_data[L].emplace<details::LocalizationIndex(L)>(std::forward<typename details::LocalizationMapping<L>::value_t>(v));
        }
        template <Localization L>
        void Add(const typename details::LocalizationMapping<L>::value_t& v)
        {
            m_data[L].emplace<details::LocalizationIndex(L)>(v);
        }

        // Return a value indicating whether the given data type is stored in the context.
        bool Contains(Localization l) { return (m_data.find(l) != m_data.end()); }

        // Gets context data
        template <Localization L>
        typename details::LocalizationMapping<L>::value_t Get() const
        {
            auto itr = m_data.find(L);
            if (itr == m_data.end())
            {
                return {};
            }
            else
            {
                return std::get<details::LocalizationIndex(L)>(itr->second);
            }
        }

    private:
        std::map<Localization, details::LocalizationVariant> m_data;
    };
}