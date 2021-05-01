#pragma once
#include "AppCatalogInfo.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalogInfo : AppCatalogInfoT<AppCatalogInfo>
    {
        AppCatalogInfo() = default;

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Arg();
        hstring ExtraData();
        Windows::Foundation::DateTime LastUpdateTime();
        Microsoft::Management::Deployment::AppCatalogOrigin Origin();
        Microsoft::Management::Deployment::AppCatalogTrustLevel TrustLevel();
    };
}
