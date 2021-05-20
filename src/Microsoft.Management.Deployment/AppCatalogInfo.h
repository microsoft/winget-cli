#pragma once
#include "AppCatalogInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalogInfo : AppCatalogInfoT<AppCatalogInfo>
    {
        AppCatalogInfo() = default;
        void Initialize(::AppInstaller::Repository::SourceDetails sourceDetails);

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Argument();
        hstring ExtraData();
        winrt::Windows::Foundation::DateTime LastUpdateTime();
        winrt::Microsoft::Management::Deployment::AppCatalogOrigin Origin();
        winrt::Microsoft::Management::Deployment::AppCatalogTrustLevel TrustLevel();
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails;
    };
}
