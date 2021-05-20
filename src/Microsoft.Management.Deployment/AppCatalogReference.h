#pragma once
#include "AppCatalogReference.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalogReference : AppCatalogReferenceT<AppCatalogReference>
    {
        AppCatalogReference() = default;
        void Initialize(winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog, winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::LocalAppCatalog localAppCatalog, winrt::Microsoft::Management::Deployment::AppCatalogInfo appCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::CreateCompositeAppCatalogOptions options);

        winrt::Microsoft::Management::Deployment::AppCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositeAppCatalogOptions m_compositeAppCatalogOptions{ nullptr };
        std::wstring m_catalogId = L"winget";
        bool m_isPredefinedSource = false;
        winrt::Microsoft::Management::Deployment::PredefinedAppCatalog m_predefinedAppCatalog = winrt::Microsoft::Management::Deployment::PredefinedAppCatalog::OpenWindowsCatalog;
        bool m_isLocalSource = false;
        winrt::Microsoft::Management::Deployment::LocalAppCatalog m_localAppCatalog = winrt::Microsoft::Management::Deployment::LocalAppCatalog::InstalledPackages;
        bool m_isCompositeSource = false;
        winrt::Microsoft::Management::Deployment::AppCatalogInfo m_info{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;
    };
}
