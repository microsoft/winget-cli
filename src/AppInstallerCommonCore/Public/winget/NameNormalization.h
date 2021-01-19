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
        Utility::Architecture Architecture() const { return m_arch; }
        const std::string& Locale() const { return m_locale; }
        const std::string& Publisher() const { return m_publisher; }

    private:
        std::string m_name;
        Utility::Architecture m_arch;
        std::string m_locale;
        std::string m_publisher;
    };

    namespace details
    {
        // p-impl for NameNormalizer
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

        // TODO: Expose test only function to ensure internal data consistency (such as sorted arrays)

    private:
        std::unique_ptr<details::INameNormalizer> m_normalizer;
    };
}
