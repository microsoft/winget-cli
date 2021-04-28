#include "pch.h"
#include "AppInstaller.h"
#include "AppInstaller.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogs()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog)
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalogById(hstring const& catalogId)
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetCompositeAppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions const& options)
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
}
