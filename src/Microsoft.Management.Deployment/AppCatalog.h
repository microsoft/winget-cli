#pragma once
#include "AppCatalog.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalog : AppCatalogT<AppCatalog>
    {
        AppCatalog() = default;

        bool IsComposite();
        Microsoft::Management::Deployment::AppCatalogInfo Info();
        Windows::Foundation::IAsyncAction OpenAsync();
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options);
    };
}
