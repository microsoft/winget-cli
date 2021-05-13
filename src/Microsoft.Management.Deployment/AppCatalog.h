#pragma once
#include "AppCatalog.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalog : AppCatalogT<AppCatalog>
    {
        AppCatalog() = default;
        void Initialize(hstring const& catalogId);
        void Initialize(Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog);
        void Initialize(Microsoft::Management::Deployment::LocalAppCatalog localAppCatalog);
        void Initialize(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions options);

        bool IsComposite();
        Microsoft::Management::Deployment::AppCatalogInfo Info();
        Windows::Foundation::IAsyncAction OpenAsync();
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(Microsoft::Management::Deployment::FindPackagesOptions options);
    private:
        Microsoft::Management::Deployment::GetCompositeAppCatalogOptions m_compositeAppCatalogOptions{ nullptr };
        std::wstring m_catalogId = L"winget";
        bool m_isPredefinedSource = false;
        Microsoft::Management::Deployment::PredefinedAppCatalog m_predefinedAppCatalog = Microsoft::Management::Deployment::PredefinedAppCatalog::OpenWindowsCatalog;
        bool m_isLocalSource = false;
        Microsoft::Management::Deployment::LocalAppCatalog m_localAppCatalog = Microsoft::Management::Deployment::LocalAppCatalog::InstalledPackages;
        bool m_isCompositeSource = false;
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;

    };
}
