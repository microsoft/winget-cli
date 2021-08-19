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
            using value_t = string_t;
        };

        template <>
        struct LocalizationMapping<Localization::Tags>
        {
            using value_t = std::vector<string_t>;
        };

        // Used to deduce the LocalizationVariant type; making a variant that includes std::monostate and all LocalizationMapping types.
        template <size_t... I>
        inline auto Deduce(std::index_sequence<I...>) { return std::variant<std::monostate, typename LocalizationMapping<static_cast<Localization>(I)>::value_t...>{}; }

        // Holds data of any type listed in a LocalizationMapping.
        using LocalizationVariant = decltype(Deduce(std::make_index_sequence<static_cast<size_t>(Localization::Max)>()));

        // Gets the index into the variant for the given Localization.
        constexpr inline size_t LocalizationIndex(Localization l) { return static_cast<size_t>(l) + 1; }
    }

    struct ManifestLocalization
    {
        string_t Locale;

        // Adds a value to the Localization data, or overwrites an existing entry.
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

        // Return a value indicating whether the given localization type exists.
        bool Contains(Localization l) const { return (m_data.find(l) != m_data.end()); }

        // Gets the localization value if exists, otherwise empty for easier access
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

        void ReplaceOrMergeWith(const ManifestLocalization& other)
        {
            for (auto const& entry : other.m_data)
            {
                this->m_data[entry.first] = entry.second;
            }

            this->Locale = other.Locale;
        }

    private:
        std::map<Localization, details::LocalizationVariant> m_data;
    };
}