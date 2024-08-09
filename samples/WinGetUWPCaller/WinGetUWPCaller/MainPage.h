// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MainPage.g.h"
#include "ActivePackageView.h"
#include <winrt\Microsoft.Management.Deployment.h>

namespace Deployment = winrt::Microsoft::Management::Deployment;

namespace winrt::WinGetUWPCaller::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> PackageCatalogs();
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> InstalledPackages();
        Windows::Foundation::Collections::IObservableVector<WinGetUWPCaller::ActivePackageView> ActivePackages();

        void LoadCatalogsButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void SearchButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void InstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void UpgradeButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void DownloadButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void CancelButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void UninstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void RefreshInstalledButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void RefreshActiveButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        Windows::Foundation::IAsyncAction GetCatalogs(winrt::Windows::UI::Xaml::Controls::Button button);
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
        Windows::Foundation::IAsyncOperation<Deployment::FindPackagesResult> TryFindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperation<Deployment::CatalogPackage> FindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> InstallPackage(Deployment::CatalogPackage package);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> DownloadPackage(Deployment::CatalogPackage package, std::wstring downloadDirectory);
        Windows::Foundation::IAsyncOperation<Deployment::PackageCatalog> FindSourceAsync(std::wstring packageSource);

        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> m_packageCatalogs;
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> m_installedPackages;
        Windows::Foundation::Collections::IObservableVector<WinGetUWPCaller::ActivePackageView> m_activePackageViews;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> m_installPackageOperation;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> m_downloadPackageOperation;
        std::wstring m_installAppId;
        std::wstring m_downloadDirectory;
        Deployment::PackageManager m_packageManager{ nullptr };
        bool m_useDev = false;
    };
}

namespace winrt::WinGetUWPCaller::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
