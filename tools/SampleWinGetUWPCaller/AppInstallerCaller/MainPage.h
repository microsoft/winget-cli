// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "InstallingPackageView.h"
#include <winrt\Microsoft.Management.Deployment.h>

#include "MainPage.g.h"
namespace Deployment = winrt::Microsoft::Management::Deployment;

namespace winrt::AppInstallerCaller::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> PackageCatalogs();
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> InstalledApps();
        Windows::Foundation::Collections::IObservableVector<winrt::AppInstallerCaller::InstallingPackageView> InstallingPackages();

        void ToggleDevSwitchToggled(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void FindSourcesButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void InstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void CancelButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void DownloadButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void DownloadCancelButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void SearchButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void RefreshInstalledButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void ClearInstalledButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void RefreshInstallingButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void ClearInstallingButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        Windows::Foundation::IAsyncAction GetSources(winrt::Windows::UI::Xaml::Controls::Button button);
        Windows::Foundation::IAsyncAction GetInstalledPackages(winrt::Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction GetInstallingPackages(winrt::Windows::UI::Xaml::Controls::TextBlock statusText);

        Windows::Foundation::IAsyncAction StartInstall(
            winrt::Windows::UI::Xaml::Controls::Button installButton,
            winrt::Windows::UI::Xaml::Controls::Button cancelButton,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction StartDownload(
            winrt::Windows::UI::Xaml::Controls::Button installButton,
            winrt::Windows::UI::Xaml::Controls::Button cancelButton,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction FindPackage(
            winrt::Windows::UI::Xaml::Controls::Button installButton,
            winrt::Windows::UI::Xaml::Controls::Button downloadButton,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText);

    private:
        Deployment::PackageManager CreatePackageManager();
        Deployment::InstallOptions CreateInstallOptions();
        Deployment::FindPackagesOptions CreateFindPackagesOptions();
        Deployment::CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions();
        Deployment::PackageMatchFilter CreatePackageMatchFilter();
        Deployment::DownloadOptions CreateDownloadOptions();

        Windows::Foundation::IAsyncOperation<Deployment::FindPackagesResult> TryFindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperation<Deployment::CatalogPackage> FindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> InstallPackage(Deployment::CatalogPackage package);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> DownloadPackage(Deployment::CatalogPackage package, std::wstring downloadDirectory);
        Windows::Foundation::IAsyncOperation<Deployment::PackageCatalog> FindSourceAsync(std::wstring packageSource);

        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> m_packageCatalogs;
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> m_installedPackages;
        Windows::Foundation::Collections::IObservableVector<winrt::AppInstallerCaller::InstallingPackageView> m_installingPackageViews;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> m_installPackageOperation;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> m_downloadPackageOperation;
        std::wstring m_installAppId;
        std::wstring m_downloadDirectory;
        Deployment::PackageManager m_packageManager{ nullptr };
        bool m_useDev = false;
    };
}

namespace winrt::AppInstallerCaller::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
