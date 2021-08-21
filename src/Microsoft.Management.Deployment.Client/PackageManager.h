#pragma once
#include "PackageManager.g.h"

const CLSID CLSID_PackageManager = { 0x74CB3139, 0xB7C5, 0x4B9E, { 0x93, 0x88, 0xE6, 0x61, 0x6D, 0xEA, 0x28, 0x8C } };  //74CB3139-B7C5-4B9E-9388-E6616DEA288C

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageManager : PackageManagerT<PackageManager>
    {
        PackageManager() = default;

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> GetPackageCatalogs();
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetPackageCatalogByName(hstring const& catalogName);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageManager : PackageManagerT<PackageManager, implementation::PackageManager>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_PackageManager, CLSCTX_ALL);
        }
    };
}
