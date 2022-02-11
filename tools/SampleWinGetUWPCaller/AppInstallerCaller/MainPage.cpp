#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

#include <wrl.h>
#include <winrt/Microsoft.Management.Deployment.h>
#include <winrt/Windows.UI.Core.h>

using namespace Microsoft::WRL;
using namespace std::chrono_literals;
using namespace winrt::Microsoft::Management::Deployment;

namespace winrt
{
    using namespace Windows::UI::Xaml;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
}

// CLSIDs for WinGet package
const CLSID CLSID_PackageManager = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14
const CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
const CLSID CLSID_PackageMatchFilter = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
const CLSID CLSID_CreateCompositePackageCatalogOptions = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37

// CLSIDs for WinGetDev package
const CLSID CLSID_PackageManager2 = { 0x74CB3139, 0xB7C5, 0x4B9E, { 0x93, 0x88, 0xE6, 0x61, 0x6D, 0xEA, 0x28, 0x8C } };  //74CB3139-B7C5-4B9E-9388-E6616DEA288C
const CLSID CLSID_InstallOptions2 = { 0x44FE0580, 0x62F7, 0x44D4, 0x9E, 0x91, 0xAA, 0x96, 0x14, 0xAB, 0x3E, 0x86 };  //44FE0580-62F7-44D4-9E91-AA9614AB3E86
const CLSID CLSID_FindPackagesOptions2 = { 0x1BD8FF3A, 0xEC50, 0x4F69, { 0xAE, 0xEE, 0xDF, 0x4C, 0x9D, 0x3B, 0xAA, 0x96 } }; //1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96
const CLSID CLSID_PackageMatchFilter2 = { 0x3F85B9F4, 0x487A, 0x4C48, { 0x90, 0x35, 0x29, 0x03, 0xF8, 0xA6, 0xD9, 0xE8 } }; //3F85B9F4-487A-4C48-9035-2903F8A6D9E8
const CLSID CLSID_CreateCompositePackageCatalogOptions2 = { 0xEE160901, 0xB317, 0x4EA7, { 0x9C, 0xC6, 0x53, 0x55, 0xC6, 0xD7, 0xD8, 0xA7 } }; //EE160901-B317-4EA7-9CC6-5355C6D7D8A7

namespace winrt::AppInstallerCaller::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        m_packageCatalogs = winrt::single_threaded_observable_vector<PackageCatalogReference>();
        m_installedPackages = winrt::single_threaded_observable_vector<CatalogPackage>();
        m_installingPackageViews = winrt::single_threaded_observable_vector<winrt::AppInstallerCaller::InstallingPackageView>();
    }

    PackageManager MainPage::CreatePackageManager() {
        if (m_useDev)
        {
            return winrt::create_instance<PackageManager>(CLSID_PackageManager2, CLSCTX_ALL);
        }
        return winrt::create_instance<PackageManager>(CLSID_PackageManager, CLSCTX_ALL);
    }
    InstallOptions MainPage::CreateInstallOptions() {
        if (m_useDev)
        {
            return winrt::create_instance<InstallOptions>(CLSID_InstallOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<InstallOptions>(CLSID_InstallOptions, CLSCTX_ALL);
    }
    FindPackagesOptions MainPage::CreateFindPackagesOptions() {
        if (m_useDev)
        {
            return winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions, CLSCTX_ALL);
    }
    CreateCompositePackageCatalogOptions MainPage::CreateCreateCompositePackageCatalogOptions() {
        if (m_useDev)
        {
            return winrt::create_instance<CreateCompositePackageCatalogOptions>(CLSID_CreateCompositePackageCatalogOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<CreateCompositePackageCatalogOptions>(CLSID_CreateCompositePackageCatalogOptions, CLSCTX_ALL);
    }
    PackageMatchFilter MainPage::CreatePackageMatchFilter() {
        if (m_useDev)
        {
            return winrt::create_instance<PackageMatchFilter>(CLSID_PackageMatchFilter2, CLSCTX_ALL);
        }
        return winrt::create_instance<PackageMatchFilter>(CLSID_PackageMatchFilter, CLSCTX_ALL);
    }

    IAsyncOperation<PackageCatalog> MainPage::FindSourceAsync(std::wstring packageSource)
    {
        PackageManager packageManager = CreatePackageManager();
        PackageCatalogReference catalogRef{ packageManager.GetPackageCatalogByName(packageSource) };
        if (catalogRef)
        {
            ConnectResult connectResult{ co_await catalogRef.ConnectAsync() };
            // PackageCatalog will be null if connectResult.ErrorCode() is a failure
            PackageCatalog catalog = connectResult.PackageCatalog();
            co_return catalog;
        }
    }

    IAsyncOperation<FindPackagesResult> MainPage::TryFindPackageInCatalogAsync(PackageCatalog catalog, std::wstring packageId)
    {
        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter = CreatePackageMatchFilter();
        filter.Field(PackageMatchField::Id);
        filter.Option(PackageFieldMatchOption::Equals);
        filter.Value(packageId);
        findPackagesOptions.Filters().Append(filter);
        return catalog.FindPackagesAsync(findPackagesOptions);
    }
    IAsyncOperation<CatalogPackage> MainPage::FindPackageInCatalogAsync(PackageCatalog catalog, std::wstring packageId)
    {
        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter = CreatePackageMatchFilter();
        filter.Field(PackageMatchField::Id);
        filter.Option(PackageFieldMatchOption::Equals);
        filter.Value(packageId);
        findPackagesOptions.Filters().Append(filter);
        FindPackagesResult findPackagesResult{ co_await catalog.FindPackagesAsync(findPackagesOptions) };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() == 0)
        {
            co_return nullptr;
        }
        co_return matches.GetAt(0).CatalogPackage();
    }

    IAsyncOperationWithProgress<InstallResult, InstallProgress> MainPage::InstallPackage(CatalogPackage package)
    {
        PackageManager packageManager = CreatePackageManager();
        InstallOptions installOptions = CreateInstallOptions();

        // Passing PackageInstallScope::User causes the install to fail if there's no installer that supports that.
        installOptions.PackageInstallScope(PackageInstallScope::Any);
        installOptions.PackageInstallMode(PackageInstallMode::Silent);

        return packageManager.InstallPackageAsync(package, installOptions);
    }

    IAsyncAction UpdateUIProgress(
        InstallProgress progress,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        co_await winrt::resume_foreground(progressBar.Dispatcher());
        progressBar.Value(progress.DownloadProgress*100);

        std::wstring downloadText{ L"Downloading. " };
        switch (progress.State)
        {
        case PackageInstallProgressState::Queued:
            statusText.Text(L"Queued");
            break;
        case PackageInstallProgressState::Downloading:
            downloadText += std::to_wstring(progress.BytesDownloaded) + L" bytes of " + std::to_wstring(progress.BytesRequired);
            statusText.Text(downloadText);
            break;
        case PackageInstallProgressState::Installing:
            statusText.Text(L"Installing");
            progressBar.IsIndeterminate(true);
            break;
        case PackageInstallProgressState::PostInstall:
            statusText.Text(L"Finishing install");
            break;
        case PackageInstallProgressState::Finished:
            statusText.Text(L"Finished install.");
            progressBar.IsIndeterminate(false);
            break;
        default:
            statusText.Text(L"");
        }
    }

    // This method is called from a background thread.
    IAsyncAction UpdateUIForInstall(
        IAsyncOperationWithProgress<InstallResult, InstallProgress> installPackageOperation,
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        installPackageOperation.Progress([=](
            IAsyncOperationWithProgress<InstallResult, InstallProgress> const& /* sender */,
            InstallProgress const& progress)
            {
                UpdateUIProgress(progress, progressBar, statusText).get();
            });

        winrt::hresult installOperationHr = S_OK;
        std::wstring errorMessage{ L"Unknown Error" };
        InstallResult installResult{ nullptr };
        try
        {
            installResult = co_await installPackageOperation;
        }
        catch (hresult_canceled const&)
        {
            errorMessage = L"Cancelled";
            OutputDebugString(L"Operation was cancelled");
        }
        catch (...)
        {
            // Operation failed
            // Example: HRESULT_FROM_WIN32(ERROR_DISK_FULL).
            installOperationHr = winrt::to_hresult();
            // Example: "There is not enough space on the disk."
            errorMessage = winrt::to_message();
            OutputDebugString(L"Operation failed");
        }

        // Switch back to ui thread context.
        co_await winrt::resume_foreground(progressBar.Dispatcher());

        cancelButton.IsEnabled(false);
        installButton.IsEnabled(true);
        progressBar.IsIndeterminate(false);

        if (installPackageOperation.Status() == AsyncStatus::Canceled)
        {
            installButton.Content(box_value(L"Retry"));
            statusText.Text(L"Install cancelled.");
        }
        if (installPackageOperation.Status() == AsyncStatus::Error || installResult == nullptr)
        {
            installButton.Content(box_value(L"Retry"));
            statusText.Text(errorMessage);
        }
        else if (installResult.RebootRequired())
        {
            installButton.Content(box_value(L"Install"));
            statusText.Text(L"Reboot to finish installation.");
        }
        else if (installResult.Status() == InstallResultStatus::Ok)
        {
            installButton.Content(box_value(L"Install"));
            statusText.Text(L"Finished.");
        }
        else
        {
            std::wostringstream failText;
            failText << L"Install failed: " << installResult.ExtendedErrorCode() << L" [" << installResult.InstallerErrorCode() << L"]";
            installButton.Content(box_value(L"Install"));
            statusText.Text(failText.str());
        }
    }

    IAsyncAction MainPage::GetSources(winrt::Windows::UI::Xaml::Controls::Button button)
    {
        co_await winrt::resume_background();

        PackageManager packageManager = CreatePackageManager();
        auto catalogs{ packageManager.GetPackageCatalogs() };
        auto storeCatalog{ packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog::MicrosoftStore) };

        co_await winrt::resume_foreground(button.Dispatcher());

        m_packageCatalogs.Clear();
        for (auto const catalog : catalogs)
        {
            m_packageCatalogs.Append(catalog);
        }
        m_packageCatalogs.Append(storeCatalog);

        co_return;
    }

    IAsyncAction MainPage::GetInstalledPackages(winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        int32_t selectedIndex = catalogsListBox().SelectedIndex();
        co_await winrt::resume_background();

        PackageManager packageManager = CreatePackageManager();

        PackageCatalogReference installedSearchCatalogRef{ nullptr };

        if (selectedIndex < 0)
        {
            installedSearchCatalogRef = packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstalledPackages);
        }
        else
        {
            PackageCatalogReference selectedRemoteCatalogRef = m_packageCatalogs.GetAt(selectedIndex);
            CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = CreateCreateCompositePackageCatalogOptions();
            createCompositePackageCatalogOptions.Catalogs().Append(selectedRemoteCatalogRef);

            createCompositePackageCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::LocalCatalogs);
            installedSearchCatalogRef = packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions);
        }

        ConnectResult connectResult{ co_await installedSearchCatalogRef.ConnectAsync() };
        PackageCatalog installedCatalog = connectResult.PackageCatalog();
        if (!installedCatalog)
        {
            // Connect Error.
            co_await winrt::resume_foreground(statusText.Dispatcher());
            statusText.Text(L"Failed to connect to catalog.");
            co_return;
        }

        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();

        FindPackagesResult findResult{ TryFindPackageInCatalogAsync(installedCatalog, m_installAppId).get() };
        auto matches = findResult.Matches();

        co_await winrt::resume_foreground(statusText.Dispatcher());
        m_installedPackages.Clear();
        for (auto const match : matches)
        {
            // Filter to only packages that match the selectedCatalogRef
            auto version = match.CatalogPackage().DefaultInstallVersion();
            if (selectedIndex < 0 || (version && version.PackageCatalog().Info().Id() == m_packageCatalogs.GetAt(selectedIndex).Info().Id()))
            {
                m_installedPackages.Append(match.CatalogPackage());
            }
        }

        statusText.Text(L"");
        co_return;
    }
    IAsyncAction MainPage::GetInstallingPackages(winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        int32_t selectedIndex = catalogsListBox().SelectedIndex();
        co_await winrt::resume_background();

        PackageManager packageManager = CreatePackageManager();

        PackageCatalogReference installingSearchCatalogRef{ nullptr };

        if (selectedIndex < 0)
        {
            // Installing package querying is only really useful if you know what Catalog you're interested in.
            co_await winrt::resume_foreground(statusText.Dispatcher());
            statusText.Text(L"No catalog selected.");
            co_return;
        }

        installingSearchCatalogRef = packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstallingPackages);

        PackageCatalogReference selectedRemoteCatalogRef = m_packageCatalogs.GetAt(selectedIndex);
        ConnectResult remoteConnectResult{ co_await selectedRemoteCatalogRef.ConnectAsync() };
        PackageCatalog selectedRemoteCatalog = remoteConnectResult.PackageCatalog();
        if (!selectedRemoteCatalog)
        {
            co_await winrt::resume_foreground(statusText.Dispatcher());
            statusText.Text(L"Failed to connect to catalog.");
            co_return;
        }

        ConnectResult connectResult{ co_await installingSearchCatalogRef.ConnectAsync() };
        PackageCatalog installingCatalog = connectResult.PackageCatalog();
        if (!installingCatalog)
        {
            co_await winrt::resume_foreground(statusText.Dispatcher());
            statusText.Text(L"Failed to connect to catalog.");
            co_return;
        }

        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();

        FindPackagesResult findResult{ TryFindPackageInCatalogAsync(selectedRemoteCatalog, m_installAppId).get() };
        auto matches = findResult.Matches();

        co_await winrt::resume_foreground(statusText.Dispatcher());

        m_installingPackageViews.Clear();
        for (auto const match : matches)
        {
            winrt::AppInstallerCaller::InstallingPackageView installingView;
            installingView.Package(match.CatalogPackage());
            auto installOperation = packageManager.GetInstallProgress(installingView.Package(), selectedRemoteCatalog.Info());
            if (installOperation)
            {
                installingView.Dispatcher(statusText.Dispatcher());
                installingView.AsyncOperation(installOperation);
                m_installingPackageViews.Append(installingView);
            }
        }

        statusText.Text(L"");
        co_return;
    }

    IAsyncAction MainPage::StartInstall(
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        installButton.IsEnabled(false);
        cancelButton.IsEnabled(true);
        int32_t selectedIndex = catalogsListBox().SelectedIndex();

        co_await winrt::resume_background();

        if (selectedIndex < 0)
        {
            co_await winrt::resume_foreground(installButton.Dispatcher());
            installButton.IsEnabled(false);
            statusText.Text(L"No catalog selected to install.");
            co_return;
        }

        // Get the remote catalog
        PackageCatalogReference selectedRemoteCatalogRef = m_packageCatalogs.GetAt(selectedIndex);
        ConnectResult remoteConnectResult{ co_await selectedRemoteCatalogRef.ConnectAsync() };
        PackageCatalog selectedRemoteCatalog = remoteConnectResult.PackageCatalog();
        if (!selectedRemoteCatalog)
        {
            co_await winrt::resume_foreground(progressBar.Dispatcher());
            statusText.Text(L"Connecting to catalog failed.");
            co_return;
        }
        FindPackagesResult findPackagesResult{ TryFindPackageInCatalogAsync(selectedRemoteCatalog, m_installAppId).get() };
        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() > 0)
        {
            m_installPackageOperation = InstallPackage(matches.GetAt(0).CatalogPackage());
            UpdateUIForInstall(m_installPackageOperation, installButton, cancelButton, progressBar, statusText);
        }
    }

    IAsyncAction MainPage::FindPackage(
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        int32_t selectedIndex = catalogsListBox().SelectedIndex();
        if (selectedIndex < 0)
        {
            co_await winrt::resume_foreground(installButton.Dispatcher());
            installButton.IsEnabled(false);
            statusText.Text(L"No catalog selected to search.");
            co_return;
        }

        co_await winrt::resume_background();

        // Get the remote catalog
        PackageCatalogReference selectedRemoteCatalogRef = m_packageCatalogs.GetAt(selectedIndex);
        // Create the composite catalog
        CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = CreateCreateCompositePackageCatalogOptions();
        createCompositePackageCatalogOptions.Catalogs().Append(selectedRemoteCatalogRef);
        PackageManager packageManager = CreatePackageManager();
        PackageCatalogReference catalogRef{ packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions) };
        ConnectResult connectResult{ co_await catalogRef.ConnectAsync() };
        PackageCatalog compositeCatalog = connectResult.PackageCatalog();
        if (!compositeCatalog)
        {
            co_await winrt::resume_foreground(installButton.Dispatcher());
            installButton.IsEnabled(false);
            statusText.Text(L"Failed to connect to catalog.");
            co_return;
        }

        // Do the search.
        FindPackagesResult findPackagesResult{ TryFindPackageInCatalogAsync(compositeCatalog, m_installAppId).get() };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() > 0)
        {
            auto installedVersion = matches.GetAt(0).CatalogPackage().InstalledVersion();
            if (installedVersion != nullptr)
            {
                co_await winrt::resume_foreground(installButton.Dispatcher());
                installButton.IsEnabled(false);
                statusText.Text(L"Already installed. Product code: " + installedVersion.ProductCodes().GetAt(0));
            }
            else
            {
                co_await winrt::resume_foreground(installButton.Dispatcher());
                installButton.IsEnabled(true);
                statusText.Text(L"Found the package to install.");
            }
        }
        else
        {
            co_await winrt::resume_foreground(installButton.Dispatcher());
            installButton.IsEnabled(false);
            statusText.Text(L"Did not find package.");
        }
        co_return;
    }

    void MainPage::SearchButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        m_installAppId = catalogIdTextBox().Text();
        installButton().IsEnabled(false);
        cancelButton().IsEnabled(false);
        installStatusText().Text(L"Looking for package.");
        FindPackage(installButton(), installProgressBar(), installStatusText());
    }
    void MainPage::InstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation == nullptr || m_installPackageOperation.Status() != AsyncStatus::Started)
        {
            StartInstall(installButton(), cancelButton(), installProgressBar(), installStatusText());
        }
    }

    void MainPage::CancelButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation && m_installPackageOperation.Status() == AsyncStatus::Started)
        {
            m_installPackageOperation.Cancel();
        }
    }
    void MainPage::RefreshInstalledButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetInstalledPackages(installedStatusText());
    }
    void MainPage::ClearInstalledButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        m_installedPackages.Clear();
    }
    void MainPage::RefreshInstallingButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetInstallingPackages(installingStatusText());
    }
    void MainPage::ClearInstallingButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        m_installingPackageViews.Clear();
    }

    void MainPage::ToggleDevSwitchToggled(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
    {
        m_useDev = toggleDevSwitch().IsOn();
    }
    void MainPage::FindSourcesButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetSources(installButton());
    }

    Windows::Foundation::Collections::IObservableVector<Microsoft::Management::Deployment::PackageCatalogReference> MainPage::PackageCatalogs()
    {
        return m_packageCatalogs;
    }
    Windows::Foundation::Collections::IObservableVector<Microsoft::Management::Deployment::CatalogPackage> MainPage::InstalledApps()
    {
        return m_installedPackages;
    }
    Windows::Foundation::Collections::IObservableVector<winrt::AppInstallerCaller::InstallingPackageView> MainPage::InstallingPackages()
    {
        return m_installingPackageViews;
    }
}
