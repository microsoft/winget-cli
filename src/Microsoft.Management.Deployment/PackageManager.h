// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageManager.g.h"
#include "Public/ComClsids.h"

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
// Forward declaration
namespace AppInstaller::CLI::Execution
{
    struct Context;
}
#endif

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_PackageManager)]
    struct PackageManager : PackageManagerT<PackageManager>
    {
        PackageManager() = default;

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> GetPackageCatalogs();
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference GetPackageCatalogByName(hstring const& catalogName);
        winrt::Microsoft::Management::Deployment::PackageCatalogReference CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> 
            InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options);
        // Contract 2.0
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> 
            GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo);
        // Contract 4.0
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress>
            UpgradePackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress>
            UninstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::UninstallOptions options);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress>
            GetUninstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo);
    };

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    void SetComCallerName(std::string name);
    void PopulateContextFromInstallOptions(AppInstaller::CLI::Execution::Context* context, winrt::Microsoft::Management::Deployment::InstallOptions options);
#endif
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageManager : PackageManagerT<PackageManager, implementation::PackageManager>
    {
    };
}
#endif
