#pragma once
#include "AppCatalog.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalog : AppCatalogT<AppCatalog>
    {
        AppCatalog() = default;
        AppCatalog(hstring const& catalogId);

        bool IsComposite();
        Microsoft::Management::Deployment::AppCatalogInfo Info();
        Windows::Foundation::IAsyncAction OpenAsync();
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options);
    private:
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;
        std::wstring m_catalogId = L"";
    };
}
