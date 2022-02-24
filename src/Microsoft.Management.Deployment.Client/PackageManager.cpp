// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <PackageManager.h>
#include <Client.PackageManager.h>
#pragma warning( pop )
#include "PackageManager.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageManager::GetPackageCatalogs()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPackageCatalogByName(hstring const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::UpgradePackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress> PackageManager::UninstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::UninstallOptions)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress> PackageManager::GetUninstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        throw hresult_not_implemented();
    }
}
