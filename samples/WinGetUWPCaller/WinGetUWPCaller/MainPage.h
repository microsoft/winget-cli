// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MainPage.g.h"
#include "ActivePackageView.h"
#include <winrt\Microsoft.Management.Deployment.h>
#include <mutex>

namespace Deployment = winrt::Microsoft::Management::Deployment;

namespace winrt::WinGetUWPCaller::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> PackageCatalogs();
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> InstalledPackages();
        Windows::Foundation::Collections::IObservableVector<WinGetUWPCaller::ActivePackageView> ActivePackages();

        // Select Catalog(s) section
        void LoadCatalogsButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        // Package Operations section
        void SearchButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void InstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void UpgradeButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void DownloadButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void CancelButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        // Installed Packages section
        void RefreshInstalledButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void UninstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        // Active Operations section
        void RefreshActiveButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

    private:
        // Ensures that the package manager object exists; potentially recreating it if requested.
        // Should be called from a background thread.
        // Is thread-safe.
        // Returns an error string if it fails.
        std::wstring EnsurePackageManager(bool forceRecreate = false);

        // Select Catalog(s) section
        Windows::Foundation::IAsyncAction LoadCatalogsAsync();

        // Package Operations section
        Windows::Foundation::IAsyncOperation<Deployment::FindPackagesResult> TryFindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperation<Deployment::CatalogPackage> FindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> InstallPackage(Deployment::CatalogPackage package);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> DownloadPackage(Deployment::CatalogPackage package, std::wstring downloadDirectory);
        Windows::Foundation::IAsyncOperation<Deployment::PackageCatalog> LoadCatalogAsync(std::wstring packageCatalog);

        Windows::Foundation::IAsyncAction StartInstall(
            Windows::UI::Xaml::Controls::Button installButton,
            Windows::UI::Xaml::Controls::Button cancelButton,
            Windows::UI::Xaml::Controls::ProgressBar progressBar,
            Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction StartDownload(
            Windows::UI::Xaml::Controls::Button installButton,
            Windows::UI::Xaml::Controls::Button cancelButton,
            Windows::UI::Xaml::Controls::ProgressBar progressBar,
            Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction FindPackage(
            Windows::UI::Xaml::Controls::Button installButton,
            Windows::UI::Xaml::Controls::Button downloadButton,
            Windows::UI::Xaml::Controls::ProgressBar progressBar,
            Windows::UI::Xaml::Controls::TextBlock statusText);

        // Installed Packages section
        Windows::Foundation::IAsyncAction GetInstalledPackagesAsync(Windows::UI::Xaml::Controls::Button button, Windows::UI::Xaml::Controls::TextBlock statusText);

        // Active Operations section
        Windows::Foundation::IAsyncAction GetActivePackagesAsync(Windows::UI::Xaml::Controls::Button button, Windows::UI::Xaml::Controls::TextBlock statusText);

        // Member fields
        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> m_packageCatalogs;
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> m_installedPackages;
        Windows::Foundation::Collections::IObservableVector<WinGetUWPCaller::ActivePackageView> m_activePackageViews;

        std::mutex m_packageManagerMutex;
        Deployment::PackageManager m_packageManager{ nullptr };
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> m_installPackageOperation;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> m_downloadPackageOperation;
    };
}

namespace winrt::WinGetUWPCaller::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
