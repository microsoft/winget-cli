#pragma once
#include "PackageCatalogInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogInfo : PackageCatalogInfoT<PackageCatalogInfo>
    {
        PackageCatalogInfo() = default;
        void Initialize(::AppInstaller::Repository::SourceDetails sourceDetails);

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Argument();
        winrt::Windows::Foundation::DateTime LastUpdateTime();
        winrt::Microsoft::Management::Deployment::PackageCatalogOrigin Origin();
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails{};
    };
}
