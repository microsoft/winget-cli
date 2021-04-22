#pragma once
#include "AppInstaller.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller>
    {
        AppInstaller() = default;

        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog>> GetAppCatalogsAsync();
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> GetAppCatalogAsync(Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog);
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> GetAppCatalogAsync(hstring catalogId);
        Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> GetCompositeAppCatalogAsync(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions options);
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller, implementation::AppInstaller>
    {
    };
}
