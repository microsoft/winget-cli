#pragma once
#include "AppInstaller.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller>
    {
        AppInstaller() = default;

        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> GetAppCatalogs();
        Microsoft::Management::Deployment::AppCatalog GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog);
        Microsoft::Management::Deployment::AppCatalog GetAppCatalogById(hstring const& catalogId);
        Microsoft::Management::Deployment::AppCatalog GetCompositeAppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions const& options);
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options);
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller, implementation::AppInstaller>
    {
    };
}
