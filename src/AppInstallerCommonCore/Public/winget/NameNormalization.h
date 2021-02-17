// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>

#include <string>
#include <string_view>


namespace AppInstaller::Utility
{
    // The specific version of normalization being used.
    enum class NormalizationVersion
    {
        Initial,
    };

    struct NameNormalizer;

    // A package publisher and name that has been normalized, allowing direct
    // comparison across versions and many other facet. Also allows use in
    // generating and Id for local packages.
    struct NormalizedName
    {
        NormalizedName() = default;

        const std::string& Name() const { return m_name; }
        void Name(std::string&& name) { m_name = std::move(name); }

        Utility::Architecture Architecture() const { return m_arch; }
        void Architecture(Utility::Architecture arch) { m_arch = arch; }

        const std::string& Locale() const { return m_locale; }
        void Locale(std::string&& locale) { m_locale = std::move(locale); }

        const std::string& Publisher() const { return m_publisher; }
        void Publisher(std::string&& publisher) { m_publisher = std::move(publisher); }

    private:
        std::string m_name;
        Utility::Architecture m_arch = Utility::Architecture::Unknown;
        std::string m_locale;
        std::string m_publisher;
    };

    namespace details
    {
        // NameNormalizer interface to allow different versions.
        struct INameNormalizer
        {
            virtual ~INameNormalizer() = default;

            virtual NormalizedName Normalize(std::string_view name, std::string_view publisher) const = 0;
        };
    }

    // Helper that manages the lifetime of the internals required to
    // execute the name normalization.
    struct NameNormalizer
    {
        NameNormalizer(NormalizationVersion version);

        NormalizedName Normalize(std::string_view name, std::string_view publisher) const;

    private:
        std::unique_ptr<details::INameNormalizer> m_normalizer;
    };
}
