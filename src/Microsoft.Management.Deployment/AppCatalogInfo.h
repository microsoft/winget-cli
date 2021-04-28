#pragma once
#include "AppCatalogInfo.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


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
