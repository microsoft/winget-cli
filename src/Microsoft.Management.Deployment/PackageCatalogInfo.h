// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogInfo.g.h"
#include <optional>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogInfo : PackageCatalogInfoT<PackageCatalogInfo>
    {
        PackageCatalogInfo() = default;
        void Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails);
        void Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails, ::AppInstaller::Repository::WellKnownSource wellKnownSource);
        void Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails, ::AppInstaller::Repository::PredefinedSource predefinedSource);
        ::AppInstaller::Repository::SourceDetails& GetSourceDetails();
        std::optional<::AppInstaller::Repository::WellKnownSource> GetWellKnownSource();
        std::optional<::AppInstaller::Repository::PredefinedSource> GetPredefinedSource();
        void SetAdditionalPackageCatalogArguments(std::string value);
        std::optional<std::string> GetAdditionalPackageCatalogArguments();

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Argument();
        winrt::Windows::Foundation::DateTime LastUpdateTime();
        winrt::Microsoft::Management::Deployment::PackageCatalogOrigin Origin();
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails{};
        std::optional<std::string> m_additionalPackageCatalogArguments;
        std::optional<::AppInstaller::Repository::WellKnownSource> m_wellKnownSource;
        std::optional<::AppInstaller::Repository::PredefinedSource> m_predefinedSource;
    };
}
