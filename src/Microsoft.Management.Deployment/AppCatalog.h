#pragma once
#include "AppCatalog.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppCatalog : AppCatalogT<AppCatalog>
    {
        AppCatalog() = default;
        void Initialize(winrt::Microsoft::Management::Deployment::AppCatalogInfo info, std::shared_ptr<::AppInstaller::Repository::ISource> source);

        bool IsComposite();
        winrt::Microsoft::Management::Deployment::AppCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(winrt::Microsoft::Management::Deployment::FindPackagesOptions options);
        winrt::Microsoft::Management::Deployment::FindPackagesResult FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options);
    private:
        winrt::Microsoft::Management::Deployment::AppCatalogInfo m_info{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;

    };
}
