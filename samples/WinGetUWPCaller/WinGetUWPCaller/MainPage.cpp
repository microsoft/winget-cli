#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

#include <winrt/Microsoft.Management.Deployment.h>

using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace winrt::Microsoft::Management::Deployment;

namespace winrt
{
    using namespace Windows::UI::Xaml;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
}

namespace winrt::WinGetUWPCaller::implementation
{
    namespace
    {
        std::wstring ConvertExceptionToStatusString(std::wstring_view context, std::exception_ptr exceptionPtr)
        {
            std::wostringstream result;

            try
            {
                std::rethrow_exception(exceptionPtr);
            }
            catch (const winrt::hresult_error& error)
            {
                result << context << L" :: " << L"0x" << std::hex << std::setw(8) << std::setfill(L'0') << error.code() << L": " << static_cast<std::wstring_view>(error.message());
            }
            catch (const std::exception& exception)
            {
                result << context << L" :: " << exception.what();
            }
            catch (...)
            {
                result << context << L" :: Unknown exception";
            }

            return std::move(result).str();
        }

        template <typename Operation>
        std::wstring RunAndReturnStatus(std::wstring_view context, Operation&& operation)
        {
            try
            {
                operation();
            }
            catch (...)
            {
                return ConvertExceptionToStatusString(context, std::current_exception());
            }

            return {};
        }

        // Helper object to control button states and status text.
        struct BackgroundActionData
        {
            // This object should be constructed on the foreground thread.
            // disabledButtons will be disabled during the operation.
            // enabledButtons will be enabled during the operation.
            // statusText will be updated with the result.
            BackgroundActionData(
                std::initializer_list<Windows::UI::Xaml::Controls::Button> disabledButtons,
                Windows::UI::Xaml::Controls::TextBlock statusText) :
                m_disabledButtons(disabledButtons),
                m_statusText(statusText)
            {
                if (m_disabledButtons.empty())
                {
                    throw std::exception("Must specify at least one disabled button.");
                }

                for (const auto& button : m_disabledButtons)
                {
                    button.IsEnabled(false);
                }

                m_statusText.Text(L"");
            }

            BackgroundActionData(
                std::initializer_list<Windows::UI::Xaml::Controls::Button> disabledButtons,
                std::initializer_list<Windows::UI::Xaml::Controls::Button> enabledButtons,
                Windows::UI::Xaml::Controls::TextBlock statusText) :
                BackgroundActionData(disabledButtons, statusText)
            {
                m_enabledButtons = enabledButtons;
                for (const auto& button : m_enabledButtons)
                {
                    button.IsEnabled(true);
                }
            }

            // This should be run on the foreground thread.
            void Finalize() const
            {
                for (const auto& button : m_disabledButtons)
                {
                    button.IsEnabled(true);
                }

                for (const auto& button : m_enabledButtons)
                {
                    button.IsEnabled(false);
                }

                m_statusText.Text(m_status);
            }

            template <typename Operation>
            void RunAndCatchStatus(std::wstring_view context, Operation&& operation)
            {
                if (m_status.empty())
                {
                    m_status = RunAndReturnStatus(context, operation);
                }
            }

            void Status(std::wstring&& value)
            {
                m_status = std::move(value);
            }

            bool Successful() const
            {
                return m_status.empty();
            }

            Windows::UI::Core::CoreDispatcher Dispatcher() const
            {
                return m_disabledButtons[0].Dispatcher();
            }

        private:
            std::vector<Windows::UI::Xaml::Controls::Button> m_disabledButtons;
            std::vector<Windows::UI::Xaml::Controls::Button> m_enabledButtons;
            Windows::UI::Xaml::Controls::TextBlock m_statusText;
            std::wstring m_status;
        };

        std::wstring MakeCompactByteString(uint64_t bytes)
        {
            static constexpr std::array<std::wstring_view, 4> s_sizeStrings = { L"B"sv, L"KB"sv, L"MB"sv, L"GB"sv };
            static constexpr size_t s_sizeIncrement = 1000;

            size_t sizeIndex = 0;
            while (sizeIndex < s_sizeStrings.size() && bytes > s_sizeIncrement)
            {
                sizeIndex += 1;
                bytes /= s_sizeIncrement;
            }

            return std::to_wstring(bytes).append(L" ").append(s_sizeStrings[sizeIndex]);
        }
    }

    MainPage::MainPage()
    {
        InitializeComponent();
        m_packageCatalogs = winrt::single_threaded_observable_vector<PackageCatalogReference>();
        m_installedPackages = winrt::single_threaded_observable_vector<CatalogPackage>();
        m_activePackageViews = winrt::single_threaded_observable_vector<WinGetUWPCaller::ActivePackageView>();
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

    void MainPage::LoadCatalogsButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        LoadCatalogsAsync();
    }

    void MainPage::CatalogSelectionChangedHandler(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&)
    {
        m_catalog = nullptr;
    }

    void MainPage::SearchButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        FindPackageAsync();
    }

    void MainPage::InstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_packageOperation == nullptr || m_packageOperation.Status() != AsyncStatus::Started)
        {
            InstallOrUpgradeAsync(false);
        }
    }

    void MainPage::UpgradeButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_packageOperation == nullptr || m_packageOperation.Status() != AsyncStatus::Started)
        {
            InstallOrUpgradeAsync(true);
        }
    }

    void MainPage::DownloadButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_packageOperation == nullptr || m_packageOperation.Status() != AsyncStatus::Started)
        {
            DownloadAsync();
        }
    }

    void MainPage::CancelButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_packageOperation && m_packageOperation.Status() == AsyncStatus::Started)
        {
            m_packageOperation.Cancel();
        }
    }

    void MainPage::RefreshInstalledButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetInstalledPackagesAsync();
    }

    void MainPage::UninstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        UninstallAsync();
    }

    void MainPage::RefreshActiveButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        GetActivePackagesAsync();
    }

    std::wstring MainPage::EnsurePackageManager(bool forceRecreate)
    {
        std::lock_guard<std::mutex> lock{ m_packageManagerMutex };

        std::wstring result;

        if (!m_packageManager || forceRecreate)
        {
            result = RunAndReturnStatus(L"Create PackageManager", [&]() {
                m_packageManager = PackageManager{};
            });
        }

        return result;
    }

    IAsyncAction MainPage::LoadCatalogsAsync()
    {
        BackgroundActionData actionData{ { loadCatalogsButton() }, catalogStatusText() };

        co_await winrt::resume_background();

        actionData.Status(EnsurePackageManager(true));

        decltype(m_packageManager.GetPackageCatalogs()) catalogs{ nullptr };
        actionData.RunAndCatchStatus(L"Load Catalogs", [&]() {
            catalogs = m_packageManager.GetPackageCatalogs();
            });

        co_await winrt::resume_foreground(actionData.Dispatcher());

        m_packageCatalogs.Clear();

        if (catalogs)
        {
            for (auto const catalog : catalogs)
            {
                m_packageCatalogs.Append(catalog);
            }
        }

        actionData.Finalize();
    }

    IAsyncAction MainPage::FindPackageAsync()
    {
        hstring queryInput = queryTextBox().Text();
        auto selectedItems = catalogsListBox().SelectedItems();
        int32_t searchType = searchField().SelectedIndex();
        PackageCatalog catalog = m_catalog;

        BackgroundActionData actionData{ { searchButton() }, operationStatusText() };

        co_await winrt::resume_background();

        actionData.Status(EnsurePackageManager());

        if (!catalog)
        {
            actionData.RunAndCatchStatus(L"Connect catalog(s)", [&]() {
                PackageCatalogReference catalogReference{ nullptr };

                if (selectedItems.Size() == 0)
                {
                    // If no items are selected, we use all available catalogs.
                    CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions;
                    createCompositePackageCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs);

                    for (const auto& item : m_packageManager.GetPackageCatalogs())
                    {
                        createCompositePackageCatalogOptions.Catalogs().Append(item);
                    }

                    catalogReference = m_packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions);
                }
                else if (selectedItems.Size() == 1)
                {
                    // If one items is selected, we can directly use this catalog.
                    catalogReference = selectedItems.GetAt(0).as<PackageCatalogReference>();
                }
                else
                {
                    // If multiple items are selected, we create a composite catalog using those catalogs.
                    CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions;
                    createCompositePackageCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs);

                    for (const auto& item : selectedItems)
                    {
                        createCompositePackageCatalogOptions.Catalogs().Append(item.as<PackageCatalogReference>());
                    }

                    catalogReference = m_packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions);
                }

                ConnectResult connectResult{ catalogReference.Connect() };

                switch (connectResult.Status())
                {
                case ConnectResultStatus::Ok: break;
                case ConnectResultStatus::CatalogError: throw std::exception{ "Catalog connection error." };
                case ConnectResultStatus::SourceAgreementsNotAccepted: throw std::exception{ "Required catalog agreements not accepted." };
                }

                catalog = connectResult.PackageCatalog();
            });
        }

        CatalogPackage package{ nullptr };

        actionData.RunAndCatchStatus(L"Find package", [&]() {
            FindPackagesOptions findPackagesOptions;
            PackageMatchFilter filter;

            switch (searchType)
            {
            case 0: // Generic query
                filter.Field(PackageMatchField::CatalogDefault);
                filter.Option(PackageFieldMatchOption::ContainsCaseInsensitive);
                break;
            case 1: // Identifier (case-insensitive)
                filter.Field(PackageMatchField::Id);
                filter.Option(PackageFieldMatchOption::EqualsCaseInsensitive);
                break;
            case 2: // Name (substring)
                filter.Field(PackageMatchField::Name);
                filter.Option(PackageFieldMatchOption::ContainsCaseInsensitive);
                break;
            }

            filter.Value(queryInput);
            findPackagesOptions.Selectors().Append(filter);
            FindPackagesResult findPackagesResult = catalog.FindPackages(findPackagesOptions);

            winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
            if (matches.Size() == 0)
            {
                throw std::exception{ "No package found matching input" };
            }
            else if (matches.Size() > 1)
            {
                throw std::exception{ "Multiple packages found matching input; refine query." };
            }
            else
            {
                package = matches.GetAt(0).CatalogPackage();
            }
        });

        if (package)
        {
            // Display the package name using the user's default localization information.
            std::wostringstream stream;
            stream << L"Found package: " <<
                static_cast<std::wstring_view>(package.DefaultInstallVersion().GetCatalogPackageMetadata().PackageName()) << L" [" <<
                static_cast<std::wstring_view>(package.Id()) << L"]";
            actionData.Status(std::move(stream).str());
        }

        co_await winrt::resume_foreground(actionData.Dispatcher());

        m_catalog = catalog;
        m_package = package;

        bool operationButtonsEnabled = static_cast<bool>(m_package);
        installButton().IsEnabled(operationButtonsEnabled);
        upgradeButton().IsEnabled(operationButtonsEnabled);
        downloadButton().IsEnabled(operationButtonsEnabled);

        actionData.Finalize();
    }

    IAsyncAction MainPage::InstallOrUpgradeAsync(bool upgrade)
    {
        PackageManager packageManager = m_packageManager;
        CatalogPackage package = m_package;
        auto progressBar = operationProgressBar();
        auto statusText = operationStatusText();

        BackgroundActionData actionData{ { installButton(), upgradeButton(), downloadButton() }, { cancelButton() }, statusText };

        co_await winrt::resume_background();

        Windows::Foundation::IAsyncOperationWithProgress<Deployment::InstallResult, Deployment::InstallProgress> packageOperation;

        actionData.RunAndCatchStatus(L"Begin install", [&]() {
            InstallOptions installOptions;

            // Passing PackageInstallScope::User causes the install to fail if there's no installer that supports that.
            installOptions.PackageInstallScope(PackageInstallScope::Any);
            installOptions.PackageInstallMode(PackageInstallMode::Silent);

            if (upgrade)
            {
                packageOperation = packageManager.UpgradePackageAsync(package, installOptions);
            }
            else
            {
                packageOperation = packageManager.InstallPackageAsync(package, installOptions);
            }
        });

        actionData.Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
            [packageOperation, this]() { m_packageOperation = packageOperation; });

        actionData.RunAndCatchStatus(L"Set progress handler", [&]() {
            packageOperation.Progress([&](
                IAsyncOperationWithProgress<InstallResult, InstallProgress> const& /* sender */,
                InstallProgress const& progress)
                {
                    actionData.Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                        [progressBar, statusText, progress]() {
                            progressBar.Value(progress.DownloadProgress * 100);

                            switch (progress.State)
                            {
                            case PackageInstallProgressState::Queued:
                                statusText.Text(L"Queued");
                                break;
                            case PackageInstallProgressState::Downloading:
                            {
                                std::wstring downloadText{ L"Downloaded " };
                                downloadText += MakeCompactByteString(progress.BytesDownloaded) + L" of " + MakeCompactByteString(progress.BytesRequired);
                                statusText.Text(downloadText);
                            }
                            break;
                            case PackageInstallProgressState::Installing:
                                statusText.Text(L"Installer running");
                                progressBar.IsIndeterminate(true);
                                break;
                            case PackageInstallProgressState::PostInstall:
                                statusText.Text(L"Post install bookkeeping");
                                break;
                            case PackageInstallProgressState::Finished:
                                statusText.Text(L"Done");
                                progressBar.IsIndeterminate(false);
                                break;
                            default:
                                statusText.Text(L"");
                            }
                        });
                });
            });

        InstallResult installResult{ nullptr };

        actionData.RunAndCatchStatus(L"Install", [&]() {
            installResult = packageOperation.get();
        });

        if (packageOperation && packageOperation.Status() == AsyncStatus::Canceled)
        {
            actionData.Status(L"Cancelled");
        }
        else if (installResult)
        {
            // Error handling for the installResult is done by first examining the Status.
            // Any status value other than Ok will have additional error detail in the
            // ExtendedErrorCode property. This HRESULT value will typically (but not always)
            // have the Windows Package Manager facility value (0xA15). The symbolic names and
            // meanings of these error codes can be found at:
            // https://github.com/microsoft/winget-cli/blob/master/doc/windows/package-manager/winget/returnCodes.md
            // or by using the winget CLI:
            // > winget error 0x8A150049
            // > winget error -- -2146762487
            switch (installResult.Status())
            {
            case InstallResultStatus::Ok:
                actionData.Status(installResult.RebootRequired() ? L"Reboot required" : L"Done");
                break;
            case InstallResultStatus::BlockedByPolicy:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically caused by system configuration applied by policy.
                actionData.Status(L"Blocked by policy");
                break;
            case InstallResultStatus::CatalogError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with an external service.
                actionData.Status(L"Catalog error");
                break;
            case InstallResultStatus::InternalError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with the Windows Package Manager code.
                actionData.Status(L"Internal error");
                break;
            case InstallResultStatus::InvalidOptions:
                // See installResult.ExtendedErrorCode for more detail.
                // This is caused by invalid input combinations.
                actionData.Status(L"Invalid options");
                break;
            case InstallResultStatus::DownloadError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically a transient network error.
                actionData.Status(L"Download error");
                break;
            case InstallResultStatus::InstallError:
                // See installResult.ExtendedErrorCode and installResult.InstallerErrorCode for more detail.
                // This is caused by an error in the installer or an issue with the system state.
                // InstallerErrorCode is the value returned by the installer technology in use for the install
                // attempt and may or may not be an HRESULT.
                actionData.Status(L"Installation error");
                break;
            case InstallResultStatus::ManifestError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is an issue with the catalog providing the package.
                actionData.Status(L"Manifest error");
                break;
            case InstallResultStatus::NoApplicableInstallers:
                // No applicable installers were available due the combination of the current system,
                // user settings, and parameters provided to the install request.
                actionData.Status(L"No applicable installers");
                break;
            case InstallResultStatus::NoApplicableUpgrade:
                // No upgrade was available due the combination of available versions, the current system,
                // user settings, and parameters provided to the upgrade request.
                actionData.Status(L"No applicable upgrade");
                break;
            case InstallResultStatus::PackageAgreementsNotAccepted:
                // The user has not accepted the agreements required by the package.
                actionData.Status(L"Package agreements not accepted");
                break;
            }
        }

        // Switch back to ui thread context.
        co_await winrt::resume_foreground(actionData.Dispatcher());

        progressBar.IsIndeterminate(false);

        actionData.Finalize();
    }

    IAsyncAction MainPage::DownloadAsync()
    {
        hstring downloadDirectory = downloadDirectoryTextBox().Text();
        PackageManager packageManager = m_packageManager;
        CatalogPackage package = m_package;
        auto progressBar = operationProgressBar();
        auto statusText = operationStatusText();

        BackgroundActionData actionData{ { installButton(), upgradeButton(), downloadButton() }, { cancelButton() }, statusText};

        co_await winrt::resume_background();

        Windows::Foundation::IAsyncOperationWithProgress<Deployment::DownloadResult, Deployment::PackageDownloadProgress> packageOperation;

        actionData.RunAndCatchStatus(L"Begin download", [&]() {
            DownloadOptions downloadOptions;

            if (!downloadDirectory.empty())
            {
                downloadOptions.DownloadDirectory(downloadDirectory);
            }

            packageOperation = packageManager.DownloadPackageAsync(package, downloadOptions);
        });

        actionData.Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
            [packageOperation, this]() { m_packageOperation = packageOperation; });

        actionData.RunAndCatchStatus(L"Set progress handler", [&]() {
            packageOperation.Progress([&](
                IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> const& /* sender */,
                PackageDownloadProgress const& progress)
                {
                    actionData.Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                        [progressBar, statusText, progress]() {
                            progressBar.Value(progress.DownloadProgress * 100);

                            switch (progress.State)
                            {
                            case PackageDownloadProgressState::Queued:
                                statusText.Text(L"Queued");
                                break;
                            case PackageDownloadProgressState::Downloading:
                            {
                                std::wstring downloadText{ L"Downloaded " };
                                downloadText += MakeCompactByteString(progress.BytesDownloaded) + L" of " + MakeCompactByteString(progress.BytesRequired);
                                statusText.Text(downloadText);
                            }
                            break;
                            case PackageDownloadProgressState::Finished:
                                statusText.Text(L"Done");
                                progressBar.IsIndeterminate(false);
                                break;
                            default:
                                statusText.Text(L"");
                            }
                        });
                });
            });

        DownloadResult downloadResult{ nullptr };

        actionData.RunAndCatchStatus(L"Download", [&]() {
            downloadResult = packageOperation.get();
        });

        if (packageOperation && packageOperation.Status() == AsyncStatus::Canceled)
        {
            actionData.Status(L"Cancelled");
        }
        else if (downloadResult)
        {
            // Error handling for the downloadResult is done by first examining the Status.
            // Any status value other than Ok will have additional error detail in the
            // ExtendedErrorCode property. This HRESULT value will typically (but not always)
            // have the Windows Package Manager facility value (0xA15). The symbolic names and
            // meanings of these error codes can be found at:
            // https://github.com/microsoft/winget-cli/blob/master/doc/windows/package-manager/winget/returnCodes.md
            // or by using the winget CLI:
            // > winget error 0x8A150049
            // > winget error -- -2146762487
            switch (downloadResult.Status())
            {
            case DownloadResultStatus::Ok:
                actionData.Status(L"Done");
                break;
            case DownloadResultStatus::BlockedByPolicy:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically caused by system configuration applied by policy.
                actionData.Status(L"Blocked by policy");
                break;
            case DownloadResultStatus::CatalogError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with an external service.
                actionData.Status(L"Catalog error");
                break;
            case DownloadResultStatus::InternalError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with the Windows Package Manager code.
                actionData.Status(L"Internal error");
                break;
            case DownloadResultStatus::InvalidOptions:
                // See installResult.ExtendedErrorCode for more detail.
                // This is caused by invalid input combinations.
                actionData.Status(L"Invalid options");
                break;
            case DownloadResultStatus::DownloadError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically a transient network error.
                actionData.Status(L"Download error");
                break;
            case DownloadResultStatus::ManifestError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is an issue with the catalog providing the package.
                actionData.Status(L"Manifest error");
                break;
            case DownloadResultStatus::NoApplicableInstallers:
                // No applicable installers were available due the combination of the current system,
                // user settings, and parameters provided to the install request.
                actionData.Status(L"No applicable installers");
                break;
            case DownloadResultStatus::PackageAgreementsNotAccepted:
                // The user has not accepted the agreements required by the package.
                actionData.Status(L"Package agreements not accepted");
                break;
            }
        }

        // Switch back to ui thread context.
        co_await winrt::resume_foreground(actionData.Dispatcher());

        progressBar.IsIndeterminate(false);

        actionData.Finalize();
    }

    IAsyncAction MainPage::GetInstalledPackagesAsync()
    {
        BackgroundActionData actionData{ { refreshInstalledButton() }, installedStatusText() };

        co_await winrt::resume_background();

        actionData.Status(EnsurePackageManager());

        PackageCatalog catalog{ nullptr };

        actionData.RunAndCatchStatus(L"Connect installed catalog", [&]() {
            PackageCatalogReference catalogReference = m_packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstalledPackages);
            ConnectResult connectResult{ catalogReference.Connect() };

            switch (connectResult.Status())
            {
            case ConnectResultStatus::Ok: break;
            case ConnectResultStatus::CatalogError: throw std::exception{ "Catalog connection error." };
            }

            catalog = connectResult.PackageCatalog();
        });

        winrt::IVectorView<MatchResult> matches;

        actionData.RunAndCatchStatus(L"Find package", [&]() {
            FindPackagesOptions findPackagesOptions;
            FindPackagesResult findPackagesResult = catalog.FindPackages(findPackagesOptions);

            matches = findPackagesResult.Matches();
        });

        co_await winrt::resume_foreground(actionData.Dispatcher());

        m_installedPackages.Clear();

        if (matches)
        {
            for (auto const& match : matches)
            {
                m_installedPackages.Append(match.CatalogPackage());
            }
        }

        actionData.Finalize();
    }

    IAsyncAction MainPage::UninstallAsync()
    {
        PackageManager packageManager = m_packageManager;

        auto progressBar = uninstallProgressBar();
        auto statusText = uninstallStatusText();

        IInspectable selectedValue = installedListBox().SelectedValue();
        CatalogPackage package{ nullptr };
        if (selectedValue)
        {
            package = selectedValue.as<CatalogPackage>();
        }
        else
        {
            statusText.Text(L"Select a package to uninstall");
            co_return;
        }

        BackgroundActionData actionData{ { uninstallButton() }, statusText };

        co_await winrt::resume_background();

        Windows::Foundation::IAsyncOperationWithProgress<Deployment::UninstallResult, Deployment::UninstallProgress> packageOperation;

        actionData.RunAndCatchStatus(L"Begin uninstall", [&]() {
            UninstallOptions uninstallOptions;

            uninstallOptions.PackageUninstallScope(PackageUninstallScope::Any);
            uninstallOptions.PackageUninstallMode(PackageUninstallMode::Silent);

            packageOperation = packageManager.UninstallPackageAsync(package, uninstallOptions);
        });

        actionData.RunAndCatchStatus(L"Set progress handler", [&]() {
            packageOperation.Progress([&](
                IAsyncOperationWithProgress<UninstallResult, UninstallProgress> const& /* sender */,
                UninstallProgress const& progress)
                {
                    actionData.Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                        [progressBar, statusText, progress]() {
                            progressBar.Value(progress.UninstallationProgress * 100);

                            switch (progress.State)
                            {
                            case PackageUninstallProgressState::Queued:
                                statusText.Text(L"Queued");
                                break;
                            case PackageUninstallProgressState::Uninstalling:
                                statusText.Text(L"Uninstaller running");
                                progressBar.IsIndeterminate(true);
                                break;
                            case PackageUninstallProgressState::PostUninstall:
                                statusText.Text(L"Post uninstall bookkeeping");
                                break;
                            case PackageUninstallProgressState::Finished:
                                statusText.Text(L"Done");
                                progressBar.IsIndeterminate(false);
                                break;
                            default:
                                statusText.Text(L"");
                            }
                        });
                });
            });

        UninstallResult uninstallResult{ nullptr };

        actionData.RunAndCatchStatus(L"Uninstall", [&]() {
            uninstallResult = packageOperation.get();
        });

        if (packageOperation && packageOperation.Status() == AsyncStatus::Canceled)
        {
            actionData.Status(L"Cancelled");
        }
        else if (uninstallResult)
        {
            // Error handling for the installResult is done by first examining the Status.
            // Any status value other than Ok will have additional error detail in the
            // ExtendedErrorCode property. This HRESULT value will typically (but not always)
            // have the Windows Package Manager facility value (0xA15). The symbolic names and
            // meanings of these error codes can be found at:
            // https://github.com/microsoft/winget-cli/blob/master/doc/windows/package-manager/winget/returnCodes.md
            // or by using the winget CLI:
            // > winget error 0x8A150049
            // > winget error -- -2146762487
            switch (uninstallResult.Status())
            {
            case UninstallResultStatus::Ok:
                actionData.Status(uninstallResult.RebootRequired() ? L"Reboot required" : L"Done");
                break;
            case UninstallResultStatus::BlockedByPolicy:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically caused by system configuration applied by policy.
                actionData.Status(L"Blocked by policy");
                break;
            case UninstallResultStatus::CatalogError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with an external service.
                actionData.Status(L"Catalog error");
                break;
            case UninstallResultStatus::InternalError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is typically an issue with the Windows Package Manager code.
                actionData.Status(L"Internal error");
                break;
            case UninstallResultStatus::InvalidOptions:
                // See installResult.ExtendedErrorCode for more detail.
                // This is caused by invalid input combinations.
                actionData.Status(L"Invalid options");
                break;
            case UninstallResultStatus::UninstallError:
                // See installResult.ExtendedErrorCode and installResult.UninstallerErrorCode for more detail.
                // This is caused by an error in the uninstaller or an issue with the system state.
                // UninstallerErrorCode is the value returned by the uninstaller technology in use for the uninstall
                // attempt and may or may not be an HRESULT.
                actionData.Status(L"Uninstallation error");
                break;
            case UninstallResultStatus::ManifestError:
                // See installResult.ExtendedErrorCode for more detail.
                // This is an issue with the catalog providing the package.
                actionData.Status(L"Manifest error");
                break;
            }
        }

        // Switch back to ui thread context.
        co_await winrt::resume_foreground(actionData.Dispatcher());

        progressBar.IsIndeterminate(false);

        actionData.Finalize();
    }

    IAsyncAction MainPage::GetActivePackagesAsync()
    {
        BackgroundActionData actionData{ { activeRefreshButton() }, activeStatusText() };

        co_await winrt::resume_background();

        actionData.Status(EnsurePackageManager());

        PackageCatalog catalog{ nullptr };

        actionData.RunAndCatchStatus(L"Connect installed catalog", [&]() {
            PackageCatalogReference catalogReference = m_packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstallingPackages);
            ConnectResult connectResult{ catalogReference.Connect() };

            switch (connectResult.Status())
            {
            case ConnectResultStatus::Ok: break;
            case ConnectResultStatus::CatalogError: throw std::exception{ "Catalog connection error." };
            }

            catalog = connectResult.PackageCatalog();
        });

        winrt::IVectorView<MatchResult> matches;

        actionData.RunAndCatchStatus(L"Find package", [&]() {
            FindPackagesOptions findPackagesOptions;
            FindPackagesResult findPackagesResult = catalog.FindPackages(findPackagesOptions);

            matches = findPackagesResult.Matches();
        });

        co_await winrt::resume_foreground(actionData.Dispatcher());

        m_activePackageViews.Clear();

        if (matches)
        {
            for (auto const& match : matches)
            {
                WinGetUWPCaller::ActivePackageView activeView;
                activeView.Package(match.CatalogPackage());
                auto installOperation = m_packageManager.GetInstallProgress(activeView.Package(), nullptr);
                if (installOperation)
                {
                    activeView.Dispatcher(actionData.Dispatcher());
                    activeView.AsyncOperation(installOperation);
                    m_activePackageViews.Append(activeView);
                }
            }
        }

        actionData.Finalize();
    }
}
