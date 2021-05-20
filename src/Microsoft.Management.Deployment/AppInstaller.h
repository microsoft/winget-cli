#pragma once
#include "AppInstaller.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("C53A4F16-787E-42A4-B304-29EFFB4BF597")]
    struct AppInstaller : AppInstallerT<AppInstaller>
    {
        AppInstaller() = default;

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::AppCatalogReference> GetAppCatalogs();
        winrt::Microsoft::Management::Deployment::AppCatalogReference GetPredefinedAppCatalog(winrt::Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog);
        winrt::Microsoft::Management::Deployment::AppCatalogReference GetLocalAppCatalog(winrt::Microsoft::Management::Deployment::LocalAppCatalog const& localAppCatalog);
        winrt::Microsoft::Management::Deployment::AppCatalogReference GetAppCatalogById(hstring const& catalogId);
        winrt::Microsoft::Management::Deployment::AppCatalogReference CreateCompositeAppCatalog(winrt::Microsoft::Management::Deployment::CreateCompositeAppCatalogOptions const& options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller, implementation::AppInstaller>
    {
    };
}
