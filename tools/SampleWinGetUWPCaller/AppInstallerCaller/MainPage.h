#pragma once

#include "InstallingPackageView.h"
#include <winrt\Microsoft.Management.Deployment.h>
//#include <winrt\Microsoft.Management.Deployment.Client.h>

#include "MainPage.g.h"
//namespace Deployment = winrt::Microsoft::Management::Deployment::Client;
namespace Deployment = winrt::Microsoft::Management::Deployment;

namespace winrt::AppInstallerCaller::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> PackageCatalogs();
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> InstalledApps();
        Windows::Foundation::Collections::IObservableVector<winrt::AppInstallerCaller::InstallingPackageView> InstallingPackages();

        void InitializeUI();
        void ToggleDevButtonClicked(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void FindSourcesButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void StartServerButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void InstallButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void CancelButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void SearchButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void RefreshButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void ClearInstalledButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void InstallingRefreshButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void ClearInstallingButtonClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        Windows::Foundation::IAsyncAction GetSources(winrt::Windows::UI::Xaml::Controls::Button button);
        Windows::Foundation::IAsyncAction GetInstalledPackages(winrt::Windows::UI::Xaml::Controls::Button button);
        Windows::Foundation::IAsyncAction GetInstallingPackages(winrt::Windows::UI::Xaml::Controls::Button button);

        Windows::Foundation::IAsyncAction InitializeInstallUI(
            std::wstring installAppId,
            winrt::Windows::UI::Xaml::Controls::Button installButton,
            winrt::Windows::UI::Xaml::Controls::Button cancelButton,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText); 
        Windows::Foundation::IAsyncAction StartInstall(
            winrt::Windows::UI::Xaml::Controls::Button installButton,
            winrt::Windows::UI::Xaml::Controls::Button cancelButton,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncAction FindPackage(
            winrt::Windows::UI::Xaml::Controls::Button button,
            winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
            winrt::Windows::UI::Xaml::Controls::TextBlock statusText);
        Windows::Foundation::IAsyncOperation<Deployment::CatalogPackage> FindPackageAsync();

    private:
        Deployment::PackageManager CreatePackageManager();
        Deployment::InstallOptions CreateInstallOptions();
        Deployment::FindPackagesOptions CreateFindPackagesOptions();
        Deployment::CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions();
        Deployment::PackageMatchFilter CreatePackageMatchFilter();

        Windows::Foundation::IAsyncOperation<Deployment::FindPackagesResult> TryFindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperation<Deployment::CatalogPackage> FindPackageInCatalogAsync(Deployment::PackageCatalog catalog, std::wstring packageId);
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> InstallPackage(Deployment::CatalogPackage package);
        Windows::Foundation::IAsyncOperation<Deployment::PackageCatalog> FindSourceAsync(std::wstring packageSource);
        Windows::Foundation::IAsyncAction StartServer();

        Windows::Foundation::Collections::IObservableVector<Deployment::PackageCatalogReference> m_packageCatalogs;
        Windows::Foundation::Collections::IObservableVector<Deployment::CatalogPackage> m_installedPackages;
        Windows::Foundation::Collections::IObservableVector<winrt::AppInstallerCaller::InstallingPackageView> m_installingPackageViews;
        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> m_installPackageOperation;
        std::wstring m_installAppId;
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
