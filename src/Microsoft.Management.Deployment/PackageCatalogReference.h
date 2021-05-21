#pragma once
#include "PackageCatalogReference.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogReference : PackageCatalogReferenceT<PackageCatalogReference>
    {
        PackageCatalogReference() = default;
        void Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog predefinedPackageCatalog, winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::LocalPackageCatalog localPackageCatalog, winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options);

        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions m_compositePackageCatalogOptions{ nullptr };
        std::wstring m_catalogId = L"winget";
        bool m_isPredefinedSource = false;
        winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog m_predefinedPackageCatalog = winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::OpenWindowsCatalog;
        bool m_isLocalSource = false;
        winrt::Microsoft::Management::Deployment::LocalPackageCatalog m_localPackageCatalog = winrt::Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages;
        bool m_isCompositeSource = false;
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo m_info{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;
    };
}
