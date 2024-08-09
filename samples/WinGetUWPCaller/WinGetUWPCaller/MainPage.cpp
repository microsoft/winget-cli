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

namespace winrt::WinGetUWPCaller::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        m_packageCatalogs = winrt::single_threaded_observable_vector<PackageCatalogReference>();
        m_installedPackages = winrt::single_threaded_observable_vector<CatalogPackage>();
        m_activePackageViews = winrt::single_threaded_observable_vector<WinGetUWPCaller::ActivePackageView>();
    }

    IAsyncOperation<PackageCatalog> MainPage::FindSourceAsync(std::wstring packageSource)
    {
        PackageManager packageManager;
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
        FindPackagesOptions findPackagesOptions;
        PackageMatchFilter filter;
        filter.Field(PackageMatchField::Id);
        filter.Option(PackageFieldMatchOption::Equals);
        filter.Value(packageId);
        findPackagesOptions.Filters().Append(filter);
        return catalog.FindPackagesAsync(findPackagesOptions);
    }
    IAsyncOperation<CatalogPackage> MainPage::FindPackageInCatalogAsync(PackageCatalog catalog, std::wstring packageId)
    {
        FindPackagesOptions findPackagesOptions;
        PackageMatchFilter filter;
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
        PackageManager packageManager;
        InstallOptions installOptions;

        // Passing PackageInstallScope::User causes the install to fail if there's no installer that supports that.
        installOptions.PackageInstallScope(PackageInstallScope::Any);
        installOptions.PackageInstallMode(PackageInstallMode::Silent);

        return packageManager.InstallPackageAsync(package, installOptions);
    }

    IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> MainPage::DownloadPackage(CatalogPackage package, std::wstring downloadDirectory)
    {
        PackageManager packageManager;
        DownloadOptions downloadOptions;

        if (!downloadDirectory.empty())
        {
            downloadOptions.DownloadDirectory(downloadDirectory);
        }

        return packageManager.DownloadPackageAsync(package, downloadOptions);
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

    IAsyncAction UpdateUIDownloadProgress(
        PackageDownloadProgress progress,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        co_await winrt::resume_foreground(progressBar.Dispatcher());
        progressBar.Value(progress.DownloadProgress * 100);

        std::wstring downloadText{ L"Downloading. " };
        switch (progress.State)
        {
        case PackageDownloadProgressState::Queued:
            statusText.Text(L"Queued");
            break;
        case PackageDownloadProgressState::Downloading:
            downloadText += std::to_wstring(progress.BytesDownloaded) + L" bytes of " + std::to_wstring(progress.BytesRequired);
            statusText.Text(downloadText);
            break;
        case PackageDownloadProgressState::Finished:
            statusText.Text(L"Finished download.");
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

    // This method is called from a background thread.
    IAsyncAction UpdateUIForDownload(
        IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> operation,
        winrt::Windows::UI::Xaml::Controls::Button downloadButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        operation.Progress([=](
            IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> const& /* sender */,
            PackageDownloadProgress const& progress)
            {
                UpdateUIDownloadProgress(progress, progressBar, statusText).get();
            });

        winrt::hresult downloadOperationHr = S_OK;
        std::wstring errorMessage{ L"Unknown Error" };
        DownloadResult downloadResult{ nullptr };
        try
        {
            downloadResult = co_await operation;
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
            downloadOperationHr = winrt::to_hresult();
            // Example: "There is not enough space on the disk."
            errorMessage = winrt::to_message();
            OutputDebugString(L"Operation failed");
        }

        // Switch back to ui thread context.
        co_await winrt::resume_foreground(progressBar.Dispatcher());

        cancelButton.IsEnabled(false);
        downloadButton.IsEnabled(true);
        progressBar.IsIndeterminate(false);

        if (operation.Status() == AsyncStatus::Canceled)
        {
            downloadButton.Content(box_value(L"Retry"));
            statusText.Text(L"Download cancelled.");
        }
        if (operation.Status() == AsyncStatus::Error || downloadResult == nullptr)
        {
            downloadButton.Content(box_value(L"Retry"));
            statusText.Text(errorMessage);
        }
        else if (downloadResult.Status() == DownloadResultStatus::Ok)
        {
            downloadButton.Content(box_value(L"Download"));
            statusText.Text(L"Finished.");
        }
        else
        {
            std::wostringstream failText;
            failText << L"Download failed: " << downloadResult.ExtendedErrorCode();
            downloadButton.Content(box_value(L"Download"));
            statusText.Text(failText.str());
        }
    }

    IAsyncAction MainPage::GetCatalogs(winrt::Windows::UI::Xaml::Controls::Button button)
    {
        co_await winrt::resume_background();

        PackageManager packageManager;
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

        PackageManager packageManager;

        PackageCatalogReference installedSearchCatalogRef{ nullptr };

        if (selectedIndex < 0)
        {
            installedSearchCatalogRef = packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstalledPackages);
        }
        else
        {
            PackageCatalogReference selectedRemoteCatalogRef = m_packageCatalogs.GetAt(selectedIndex);
            CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions;
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

        FindPackagesOptions findPackagesOptions;

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

        PackageManager packageManager;

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

        FindPackagesOptions findPackagesOptions;

        FindPackagesResult findResult{ TryFindPackageInCatalogAsync(selectedRemoteCatalog, m_installAppId).get() };
        auto matches = findResult.Matches();

        co_await winrt::resume_foreground(statusText.Dispatcher());

        m_activePackageViews.Clear();
        for (auto const match : matches)
        {
            WinGetUWPCaller::ActivePackageView activeView;
            activeView.Package(match.CatalogPackage());
            auto installOperation = packageManager.GetInstallProgress(activeView.Package(), selectedRemoteCatalog.Info());
            if (installOperation)
            {
                activeView.Dispatcher(statusText.Dispatcher());
                activeView.AsyncOperation(installOperation);
                m_activePackageViews.Append(activeView);
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

    IAsyncAction MainPage::StartDownload(
        winrt::Windows::UI::Xaml::Controls::Button button,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        button.IsEnabled(false);
        cancelButton.IsEnabled(true);
        int32_t selectedIndex = catalogsListBox().SelectedIndex();

        co_await winrt::resume_background();

        if (selectedIndex < 0)
        {
            co_await winrt::resume_foreground(button.Dispatcher());
            button.IsEnabled(false);
            statusText.Text(L"No catalog selected to download.");
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
            m_downloadPackageOperation = DownloadPackage(matches.GetAt(0).CatalogPackage(), m_downloadDirectory);
            UpdateUIForDownload(m_downloadPackageOperation, button, cancelButton, progressBar, statusText);
        }
    }

    IAsyncAction MainPage::FindPackage(
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button downloadButton,
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
        CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions;
        createCompositePackageCatalogOptions.Catalogs().Append(selectedRemoteCatalogRef);
        PackageManager packageManager;
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
                downloadButton.IsEnabled(true);
                statusText.Text(L"Found the package to install or download.");
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
        m_installAppId = queryTextBox().Text();
        installButton().IsEnabled(false);
        downloadButton().IsEnabled(false);
        cancelButton().IsEnabled(false);
        operationStatusText().Text(L"Looking for package.");
        FindPackage(installButton(), downloadButton(), operationProgressBar(), operationStatusText());
    }

    void MainPage::InstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation == nullptr || m_installPackageOperation.Status() != AsyncStatus::Started)
        {
            StartInstall(installButton(), cancelButton(), operationProgressBar(), operationStatusText());
        }
    }

    void MainPage::UpgradeButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        throw_hresult(E_NOTIMPL);
    }

    void MainPage::DownloadButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        m_downloadDirectory = downloadDirectoryTextBox().Text();

        if (m_downloadPackageOperation == nullptr || m_downloadPackageOperation.Status() != AsyncStatus::Started)
        {
            StartDownload(downloadButton(), cancelButton(), operationProgressBar(), operationStatusText());
        }
    }

    void MainPage::CancelButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation && m_installPackageOperation.Status() == AsyncStatus::Started)
        {
            m_installPackageOperation.Cancel();
        }
    }

    void MainPage::UninstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        throw_hresult(E_NOTIMPL);
    }

    void MainPage::RefreshInstalledButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetInstalledPackages(installedStatusText());
    }
    void MainPage::RefreshActiveButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetInstallingPackages(operationStatusText());
    }

    void MainPage::LoadCatalogsButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetCatalogs(installButton());
    }

    Windows::Foundation::Collections::IObservableVector<Microsoft::Management::Deployment::PackageCatalogReference> MainPage::PackageCatalogs()
    {
        return m_packageCatalogs;
    }
    Windows::Foundation::Collections::IObservableVector<Microsoft::Management::Deployment::CatalogPackage> MainPage::InstalledPackages()
    {
        return m_installedPackages;
    }
    Windows::Foundation::Collections::IObservableVector<WinGetUWPCaller::ActivePackageView> MainPage::ActivePackages()
    {
        return m_activePackageViews;
    }
}
