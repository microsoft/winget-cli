#pragma once
#include "AppInstaller.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("C53A4F16-787E-42A4-B304-29EFFB4BF597")]
    struct AppInstaller : AppInstallerT<AppInstaller>
    {
        AppInstaller() = default;

        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> GetAppCatalogs();
        Microsoft::Management::Deployment::AppCatalog GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog);
        Microsoft::Management::Deployment::AppCatalog GetAppCatalogByLocalAppCatalog(Microsoft::Management::Deployment::LocalAppCatalog const& localAppCatalog);
        Microsoft::Management::Deployment::AppCatalog GetAppCatalogById(hstring const& catalogId);
        Microsoft::Management::Deployment::AppCatalog GetCompositeAppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions const& options);
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options);
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package);
        
    private:
        static std::mutex g_executionLock;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller, implementation::AppInstaller>
    {
    };
}
