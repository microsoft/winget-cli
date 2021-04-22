#include "pch.h"
#include "AppInstaller.h"
#include "AppInstaller.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog>> AppInstaller::GetAppCatalogsAsync()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogAsync(Microsoft::Management::Deployment::PredefinedAppCatalog predefinedAppCatalog)
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogAsync(hstring catalogId)
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperation<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetCompositeAppCatalogAsync(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions options)
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        throw hresult_not_implemented();
    }
}
