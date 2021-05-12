#pragma once
#include "AppCatalogInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("d12a10a1-f371-4764-880d-b3097e303991")]
    struct AppCatalogInfo : AppCatalogInfoT<AppCatalogInfo>
    {
        AppCatalogInfo() = default;
        AppCatalogInfo(::AppInstaller::Repository::SourceDetails sourceDetails);

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Arg();
        hstring ExtraData();
        Windows::Foundation::DateTime LastUpdateTime();
        Microsoft::Management::Deployment::AppCatalogOrigin Origin();
        Microsoft::Management::Deployment::AppCatalogTrustLevel TrustLevel();

        void Initialize(::AppInstaller::Repository::SourceDetails sourceDetails);
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails;
    };
}
