#pragma once
#include "AppCatalog.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalog : AppCatalogT<AppCatalog>
    {
        AppCatalog() = default;
        AppCatalog(hstring const& catalogId);
        AppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog);
        AppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions options);

        bool IsComposite();
        Microsoft::Management::Deployment::AppCatalogInfo Info();
        Windows::Foundation::IAsyncAction OpenAsync();
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options);
    private:
        Microsoft::Management::Deployment::GetCompositeAppCatalogOptions m_compositeAppCatalogOptions{ nullptr };
        bool m_isPredefinedSource = false;
        std::wstring m_catalogId = L"";
        Microsoft::Management::Deployment::PredefinedAppCatalog m_predefinedAppCatalog = Microsoft::Management::Deployment::PredefinedAppCatalog::OpenWindowsCatalog;
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;

    };
}
