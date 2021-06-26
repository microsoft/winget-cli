#include "pch.h"
#include "PackageManager.h"
#include "PackageManager.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageManager::GetPackageCatalogs()
    {
        m_packageManager = winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_PackageManager2, CLSCTX_ALL);
        return m_packageManager.GetPackageCatalogs();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog)
    {
        return m_packageManager.GetPredefinedPackageCatalog(predefinedPackageCatalog);
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog)
    {
        return m_packageManager.GetLocalPackageCatalog(localPackageCatalog);
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPackageCatalogByName(hstring const& catalogName)
    {
        return m_packageManager.GetPackageCatalogByName(catalogName);
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
    {
        return m_packageManager.CreateCompositePackageCatalog(options);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        return m_packageManager.InstallPackageAsync(package, options);
    }
}
