# 1. Background

The Windows Package Manager currently exposes a command line interface to search for packages,
install them, view progress, and more. This API is designed to provide another way for callers to
make use of that functionality. The API will be preferred by callers that want to receive progress
and completion events, and UWP packages that do not have permission to launch command line processes.
The goal for this api is to provide the full set of install functionality possible using the Windows
Package Manager command line. The command line is documented at
https://docs.microsoft.com/windows/package-manager/winget/

# 2. Description

Windows Package Manager is a package manager for windows applications. It comes with a predefined
repository of applications and users can add new repositories using the winget command line. This
API allows packaged apps with the packageManagement capability and other higher privilege processes
to start, manage, and monitor installation of packages that are listed in Windows Package Manager
repositories.

# 3. Examples

Sample member values for the following examples:
m_installAppId = L"Microsoft.VisualStudioCode";

## 3.1. Create objects

Creation of objects has to be done through CoCreateInstance rather than normal winrt initialization
since it's hosted by an out of proc com server. These helper methods will be used in the rest of the
examples.

```c++ (C++ish pseudocode)
    AppInstaller CreateAppInstaller() {
        return winrt::create_instance<AppInstaller>(CLSID_AppInstaller, CLSCTX_ALL);
    }
    InstallOptions CreateInstallOptions() {
        return winrt::create_instance<InstallOptions>(CLSID_InstallOptions, CLSCTX_ALL);
    }
    FindPackagesOptions CreateFindPackagesOptions() {
        return winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions, CLSCTX_ALL);
    }
    CreateCompositeAppCatalogOptions CreateCreateCompositeAppCatalogOptions() {
        return winrt::create_instance<CreateCompositeAppCatalogOptions>(CLSID_CreateCompositeAppCatalogOptions, CLSCTX_ALL);
    }
    PackageMatchFilter CreatePackageMatchFilter() {
        return winrt::create_instance<PackageMatchFilter>(CLSID_PackageMatchFilter, CLSCTX_ALL);
    }
```

## 3.2. Search

The api can be used to search for packages in a catalog known to Windows Package Manager. This can
be used to get availability information or start an install.

```c++ (C++ish pseudocode)

    // Sample of using synchronous methods on background thread.
    CatalogPackage MainPage::FindPackageOnBackgroundThread()
    {
        PackageManager packageManager = CreatePackageManager();
        PackageCatalogReference catalogRef{
            packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog::OpenWindowsCatalog) };
        ConnectResult connectResult = catalogRef.Connect();
        if (connectResult.Status() != ConnectResultStatus::Ok)
        {
            return nullptr;
        }
        PackageCatalog catalog = connectResult.PackageCatalog();

        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter = CreatePackageMatchFilter();
        filter.Field(PackageMatchField::Id);
        filter.Option(PackageFieldMatchOption::Equals);
        filter.Value(m_installAppId);
        findPackagesOptions.Filters().Append(filter);
        // We've already switched to a background thread, so do everything synchronously.
        FindPackagesResult findPackagesResult{ catalog.FindPackages(findPackagesOptions) };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() == 0)
        {
            return nullptr;
        }
        return matches.GetAt(0).CatalogPackage();
    }

    // Sample of using async methods.
    IAsyncOperation<CatalogPackage> MainPage::FindPackageInCatalogAsync(PackageCatalog catalog,
            std::wstring packageId)
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

    IAsyncOperation<CatalogPackage> MainPage::FindPackageAsync()
    {
        PackageManager packageManager = CreatePackageManager();
        PackageCatalogReference catalogRef{
            packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog::OpenWindowsCatalog) };
        ConnectResult connectResult = catalogRef.Connect();
        if (connectResult.Status() != ConnectResultStatus::Ok)
        {
            co_return nullptr;
        }
        PackageCatalog catalog = connectResult.PackageCatalog();
        co_return FindPackageInCatalogAsync(catalog, m_installAppId).get();
    }
```

## 3.3. Install

```c++ (C++ish pseudocode)

    IAsyncOperationWithProgress<InstallResult, InstallProgress> MainPage::InstallPackage(CatalogPackage package)
    {
        PackageManager packageManager = CreatePackageManager();
        InstallOptions installOptions = CreateInstallOptions();
        installOptions.PackageInstallScope(PackageInstallScope::Any);

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
        co_return;
    }

    // This method is called from a background thread.
    IAsyncAction UpdateUIForInstall(
        IAsyncOperationWithProgress<InstallResult, InstallProgress> installPackageOperation,
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        if (installPackageOperation)
        {

            installPackageOperation.Progress([=](
                IAsyncOperationWithProgress<InstallResult, InstallProgress> const& /* sender */,
                InstallProgress const& progress)
                {
                    UpdateUIProgress(progressBar, statusText, 50, stateStr).get();
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
                installButton.Content(box_value(L"Install"));
                statusText.Text(L"Install failed.");
            }
        }
    }

    IAsyncAction MainPage::StartInstall(
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        installButton.IsEnabled(false);
        cancelButton.IsEnabled(true);

        co_await winrt::resume_background();

        PackageManager packageManager = CreatePackageManager();
        PackageCatalogReference catalogRef{
            packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog::OpenWindowsCatalog) };
        ConnectResult connectResult = catalogRef.Connect();
        if (connectResult.Status() != ConnectResultStatus::Ok)
        {
            co_await winrt::resume_foreground(progressBar.Dispatcher());
            statusText.Text(L"Connecting to catalog failed.");
            co_return;
        }
        PackageCatalog catalog = connectResult.PackageCatalog();

        FindPackagesResult findPackagesResult{ FindPackageOnBackgroundThread(catalog, m_installAppId) };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() > 0)
        {
            m_installPackageOperation = InstallPackage(matches.GetAt(0).CatalogPackage());
            UpdateUIForInstall(m_installPackageOperation, installButton, cancelButton, progressBar, statusText);
        }
        else
        {
            co_await winrt::resume_foreground(progressBar.Dispatcher());
            statusText.Text(L"Could not find package.");
            co_return;
        }
    }
```

## 3.4.1 Cancel

The async operation can be stored, or the install code can wait on an event that can be triggered.

```c++ (C++ish pseudocode)
    void MainPage::CancelButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation)
        {
            m_installPackageOperation.Cancel();
        }
    }
```

## 3.5. Open a catalog by name

Open a catalog known to the caller. There is no way to use the api to add a catalog, that must be done
on the command line.

```c++ (C++ish pseudocode)
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
```

# 4 Remarks

Notes have been added inline throughout the api details.


For this api there are multiple similar apis that are
relevant with regard to naming and consistency. There is the Windows Package Manager command line which uses
"source" to describe the various repositories that can host packages and "search" to describe looking up an app.
https://docs.microsoft.com/windows/package-manager/winget/
There is the Windows::ApplicationModel::PackageCatalog which exists as a Windows API for installing packages
and monitoring their installation progress.
https://docs.microsoft.com/uwp/api/windows.applicationmodel.packagecatalog?view=winrt-19041
And there is Windows.Management.Deployment.PackageManager which allows packages with the packageManagement
capability to install msix apps and uses "Find" to describe looking up an app
https://docs.microsoft.com/uwp/api/windows.management.deployment.packagemanager?view=winrt-19041

This API has aligned with those Windows APIs in using \*Catalog and Find.

# 5 API Details

```c# (but really MIDL3)
namespace Microsoft.Management.Deployment
{
    [contractversion(1)]
    apicontract WindowsPackageManagerContract{};

    /// State of the install.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageInstallProgressState
    {
        /// The install is queued but not yet active. Cancellation of the IAsyncOperationWithProgress in this
        /// state will prevent the package from downloading or installing.
        Queued,
        /// The installer is downloading. Cancellation of the IAsyncOperationWithProgress in this state will
        /// end the download and prevent the package from installing.
        Downloading,
        /// The install is in progress. Cancellation of the IAsyncOperationWithProgress in this state will not
        /// stop the installation or the post install cleanup.
        Installing,
        /// The installer has completed and cleanup actions are in progress. Cancellation of the
        /// IAsyncOperationWithProgress in this state will not stop cleanup or roll back the install.
        PostInstall,
        /// The operation has completed.
        Finished,
    };

    /// Progress object for the install
    /// DESIGN NOTE: percentage for the install as a whole is purposefully not included as there is no way to
    /// estimate progress when the installer is running.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    struct InstallProgress
    {
        /// State of the install
        PackageInstallProgressState State;
        /// DESIGN NOTE: BytesDownloaded may only be available for downloads done by Windows Package Manager itself.
        /// Number of bytes downloaded if known
        UInt64 BytesDownloaded;
        /// DESIGN NOTE: BytesRequired may only be available for downloads done by Windows Package Manager itself.
        /// Number of bytes required if known
        UInt64 BytesRequired;
        /// Download percentage completed
        Double DownloadProgress;
        /// Install percentage if known.
        Double InstallationProgress;
    };

    /// Status of the Install call
    /// Implementation Note: Errors mapped from AppInstallerErrors.h
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum InstallResultStatus
    {
        Ok,
        BlockedByPolicy,
        CatalogError,
        InternalError,
        InvalidOptions,
        DownloadError,
        InstallError,
        ManifestError,
        NoApplicableInstallers,
    };

    /// Result of the install
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass InstallResult
    {
        /// Used by a caller to correlate the install with a caller's data.
        String CorrelationData{ get; };
        /// Whether a restart is required to complete the install.
        Boolean RebootRequired{ get; };

        /// Batched error code, example APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED
        InstallResultStatus Status{ get; };
        /// Specific error if known, from downloader or installer itself, example ERROR_INSTALL_PACKAGE_REJECTED
        HRESULT ExtendedErrorCode{ get; };
    }

    /// IMPLEMENTATION NOTE: SourceOrigin from AppInstallerRepositorySource.h
    /// Defines the origin of the package catalog details.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageCatalogOrigin
    {
        /// Predefined means it came as part of the Windows Package Manager package and cannot be removed.
        Predefined,
        /// User means it was added by the user and could be removed.
        User,
    };

    /// IMPLEMENTATION NOTE: SourceTrustLevel from AppInstallerRepositorySource.h
    /// Defines the trust level of the package catalog.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageCatalogTrustLevel
    {
        None,
        Trusted,
    };

    /// IMPLEMENTATION NOTE: SourceDetails from AppInstallerRepositorySource.h
    /// Interface for retrieving information about an package catalog without acting on it.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageCatalogInfo
    {
        /// The package catalog's unique identifier.
        /// SAMPLE VALUES: For OpenWindowsCatalog "Microsoft.Winget.Source_8wekyb3d8bbwe"
        /// For contoso sample on msdn "contoso"
        String Id { get; };
        /// The name of the package catalog.
        /// SAMPLE VALUES: For OpenWindowsCatalog "winget".
        /// For contoso sample on msdn "contoso"
        String Name { get; };
        /// The type of the package catalog.
        /// ALLOWED VALUES: "Microsoft.Rest", "Microsoft.PreIndexed.Package"
        /// SAMPLE VALUES: For OpenWindowsCatalog "Microsoft.PreIndexed.Package".
        /// For contoso sample on msdn "Microsoft.PreIndexed.Package"
        String Type { get; };
        /// The argument used when adding the package catalog.
        /// SAMPLE VALUES: For OpenWindowsCatalog "https://winget.azureedge.net/cache"
        /// For contoso sample on msdn "https://pkgmgr-int.azureedge.net/cache"
        String Argument { get; };
        /// The last time that this package catalog was updated.
        Windows.Foundation.DateTime LastUpdateTime { get; };
        /// The origin of the package catalog.
        PackageCatalogOrigin Origin { get; };
        /// The trust level of the package catalog
        PackageCatalogTrustLevel TrustLevel { get; };
    }

    /// A metadata item of a package version.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageVersionMetadataField
    {
        /// The InstallerType of an installed package
        InstallerType,
        /// The Scope of an installed package
        InstalledScope,
        /// The system path where the package is installed
        InstalledLocation,
        /// The standard uninstall command; which may be interactive
        StandardUninstallCommand,
        /// An uninstall command that should be non-interactive
        SilentUninstallCommand,
        /// The publisher of the package
        PublisherDisplayName,
    };

    /// IMPLEMENTATION NOTE: IPackageVersion from AppInstallerRepositorySearch.h
    /// A single package version.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageVersionInfo
    {
        /// IMPLEMENTATION NOTE: PackageVersionMetadata fields from AppInstallerRepositorySearch.h
        /// Gets any metadata associated with this package version.
        /// Primarily stores data on installed packages.
        /// Metadata fields may have no value (e.g. packages that aren't installed will not have an InstalledLocation).
        String GetMetadata(PackageVersionMetadataField metadataField);
        /// IMPLEMENTATION NOTE: PackageVersionProperty fields from AppInstallerRepositorySearch.h
        String Id { get; };
        String DisplayName { get; };
        String Version { get; };
        String Channel { get; };
        /// DESIGN NOTE: RelativePath from AppInstallerRepositorySearch.h is excluded as not needed.
        /// String RelativePath;

        /// IMPLEMENTATION NOTE: PackageVersionMultiProperty fields from AppInstallerRepositorySearch.h
        /// PackageFamilyName and ProductCode can have multiple values.
        Windows.Foundation.Collections.IVectorView<String> PackageFamilyNames { get; };
        Windows.Foundation.Collections.IVectorView<String> ProductCodes { get; };

        /// Gets the package catalog  where this package version is from.
        PackageCatalog PackageCatalog { get; };

        /// DESIGN NOTE:
        /// GetManifest from IPackageVersion in AppInstallerRepositorySearch is not implemented in V1. That class has
        /// a lot of fields and no one requesting it.
        /// Gets the manifest of this package version.
        /// virtual Manifest::Manifest GetManifest() = 0;
    }

    /// IMPLEMENTATION NOTE: PackageVersionKey from AppInstallerRepositorySearch.h
    /// A key to identify a package version within a package.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageVersionId
    {
        /// The package catalog id that this version came from.
        String PackageCatalogId { get; };
        /// The version.
        String Version { get; };
        /// The channel.
        String Channel { get; };
    };

    /// IMPLEMENTATION NOTE: IPackage from AppInstallerRepositorySearch.h
    /// A package, potentially containing information about it's local state and the available versions.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass CatalogPackage
    {
        /// IMPLEMENTATION NOTE: PackageProperty fields from AppInstallerRepositorySearch.h
        /// Gets a property of this package.
        String Id { get; };
        String Name { get; };

        /// Gets the installed package information if the package is installed.
        PackageVersionInfo InstalledVersion{ get; };

        /// Gets all available versions of this package. Ordering is not guaranteed.
        Windows.Foundation.Collections.IVectorView<PackageVersionId> AvailableVersions { get; };

        /// Gets the version of this package that will be installed if version is not set in InstallOptions.
        PackageVersionInfo DefaultInstallVersion { get; };

        /// Gets a specific version of this package.
        PackageVersionInfo GetPackageVersionInfo(PackageVersionId versionKey);

        /// Gets a value indicating whether an available version is newer than the installed version.
        Boolean IsUpdateAvailable { get; };

        /// DESIGN NOTE:
        /// IsSame from IPackage in AppInstallerRepositorySearch is not implemented in V1.
        /// Determines if the given IPackage refers to the same package as this one.
        /// virtual bool IsSame(const IPackage*) const = 0;
    }

    /// IMPLEMENTATION NOTE: CompositeSearchBehavior from AppInstallerRepositorySource.h
    /// Search behavior for composite catalogs.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum CompositeSearchBehavior
    {
        /// Search local catalogs only
        LocalCatalogs,
        /// Search remote catalogs only, don't check local catalogs for InstalledVersion
        RemotePackagesFromRemoteCatalogs,
        /// Search remote catalogs, and check local catalogs for InstalledVersion
        RemotePackagesFromAllCatalogs,
        /// Search both local and remote catalogs.
        AllCatalogs,
    };

    /// IMPLEMENTATION NOTE: PackageFieldMatchOption from AppInstallerRepositorySearch.h
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageFieldMatchOption
    {
        Equals,
        EqualsCaseInsensitive,
        StartsWithCaseInsensitive,
        ContainsCaseInsensitive,
    };

    /// IMPLEMENTATION NOTE: PackageFieldMatchOption from AppInstallerRepositorySearch.h
    /// The field to match on.
    /// The values must be declared in order of preference in search results.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageMatchField
    {
        CatalogDefault,
        Id,
        Name,
        Moniker,
        Command,
        Tag,
        /// DESIGN NOTE: The following PackageFieldMatchOption from AppInstallerRepositorySearch.h are not implemented in V1.
        /// PackageFamilyName,
        /// ProductCode,
        /// NormalizedNameAndPublisher,
    };

    /// IMPLEMENTATION NOTE: PackageMatchFilter from AppInstallerRepositorySearch.h
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageMatchFilter
    {
        PackageMatchFilter();
        /// The type of string comparison for matching
        PackageFieldMatchOption Option;
        /// The field to search
        PackageMatchField Field;
        /// The value to match
        String Value;
        /// DESIGN NOTE: "Additional" from RequestMatch AppInstallerRepositorySearch.h is not implemented here.
    }

    /// IMPLEMENTATION NOTE: MatchResult from AppInstallerRepositorySearch.h
    /// A single result from the search.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass MatchResult
    {
        /// The package found by the search request.
        CatalogPackage CatalogPackage { get; };

        /// The highest order field on which the package matched the search.
        PackageMatchFilter MatchCriteria { get; };
    }

    /// Status of the FindPackages call
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum FindPackagesResultStatus
    {
        Ok,
        BlockedByPolicy,
        CatalogError,
        InternalError,
        InvalidOptions
    };

    /// IMPLEMENTATION NOTE: SearchResult from AppInstallerRepositorySearch.h
    /// Search result data returned from FindPackages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass FindPackagesResult
    {
        /// Error codes
        FindPackagesResultStatus Status{ get; };

        /// The full set of results from the search.
        Windows.Foundation.Collections.IVectorView<MatchResult> Matches { get; };

        /// If true, the results were truncated by the given ResultLimit
        /// USAGE NOTE: Windows Package Manager does not support result pagination, there is no way to continue
        /// getting more results.
        Boolean WasLimitExceeded{ get; };
    }

    /// Options for FindPackages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass FindPackagesOptions
    {
        FindPackagesOptions();

        /// DESIGN NOTE:
        /// This class maps to SearchRequest from  AppInstallerRepositorySearch.h
        /// That class is a container for data used to filter the available manifests in an package catalog.
        /// Its properties can be thought of as:
        /// (Query || Selectors...) && Filters...
        /// If Query and Selectors are both empty, the starting data set will be the entire database.
        /// Everything && Filters...
        /// Query is PackageMatchField::CatalogDefault and in the Selector list.
        /// USAGE NOTE: Only one selector with PackageMatchField::CatalogDefault is allowed.

        /// Selectors = you have to match at least one selector (if there are no selectors, then nothing is selected)
        Windows.Foundation.Collections.IVector<PackageMatchFilter> Selectors { get; };
        /// Filters = you have to match all filters(if there are no filters, then there is no filtering of selected items)
        Windows.Foundation.Collections.IVector<PackageMatchFilter> Filters{ get; };

        /// Restricts the length of the returned results to the specified count.
        UInt32 ResultLimit;
    }

    /// IMPLEMENTATION NOTE: ISource from AppInstallerRepositorySource.h
    /// A catalog for searching for packages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageCatalog
    {
        /// Gets a value indicating whether this package catalog is a composite of other package catalogs,
        /// and thus the packages may come from disparate package catalogs as well.
        Boolean IsComposite { get; };
        /// The details of the package catalog if it is not a composite.
        PackageCatalogInfo Info { get; };

        /// Searches for Packages in the catalog.
        Windows.Foundation.IAsyncOperation<FindPackagesResult> FindPackagesAsync(FindPackagesOptions options);
        FindPackagesResult FindPackages(FindPackagesOptions options);
    }

    /// Status of the Connect call
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum ConnectResultStatus
    {
        Ok,
        CatalogError,
    };

    /// Result of the Connect call
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass ConnectResult
    {
        /// Error codes
        ConnectResultStatus Status{ get; };

        PackageCatalog PackageCatalog { get; };
    }

    /// A reference to a catalog that callers can try to Connect.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageCatalogReference
    {
        /// Gets a value indicating whether this package catalog is a composite of other package catalogs,
        /// and thus the packages may come from disparate package catalogs as well.
        Boolean IsComposite { get; };
        /// The details of the package catalog if it is not a composite.
        PackageCatalogInfo Info { get; };

        /// Opens a catalog. Required before searching. For remote catalogs (i.e. not Installed and Installing) this
        /// may require downloading information from a server.
        Windows.Foundation.IAsyncOperation<ConnectResult> ConnectAsync();
        ConnectResult Connect();
    }

    /// Catalogs with PackageCatalogOrigin Predefined
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PredefinedPackageCatalog
    {
        OpenWindowsCatalog,
    };

    /// Local Catalogs with PackageCatalogOrigin Predefined
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum LocalPackageCatalog
    {
        InstalledPackages,
    };

    /// Options for creating a composite catalog.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass CreateCompositePackageCatalogOptions
    {
        CreateCompositePackageCatalogOptions();

        /// Create a composite catalog to allow searching a user defined or pre defined source
        /// and a local source (Installed packages) together
        IVector<PackageCatalogReference> Catalogs { get; };
        /// Sets the default search behavior if the catalog is a composite catalog.
        CompositeSearchBehavior CompositeSearchBehavior;
    }

    /// Required install scope for the package. If the package does not have an installer that
    /// supports the specified scope the Install call will fail with InstallResultStatus.NoApplicableInstallers
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageInstallScope
    {
        /// An installer with any install scope is valid.
        Any,
        /// Only User install scope installers are valid
        User,
        /// Only System installers will be valid
        System,
    };

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageInstallMode
    {
        /// The default experience for the installer. Installer may show some UI.
        Default,
        /// Runs the installer in silent mode. This suppresses the installer's UI to the extent
        /// possible (installer may still show some required UI).
        Silent,
        /// Runs the installer in interactive mode.
        Interactive,
    };

    /// Options when installing a package.
    /// Intended to allow full compatibility with the "winget install" command line interface.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass InstallOptions
    {
        InstallOptions();

        /// Optionally specifies the version from the package to install. If unspecified the version matching
        /// CatalogPackage.GetLatestVersion() is used.
        PackageVersionId PackageVersionId;

        /// Specifies alternate location to install package (if supported).
        String PreferredInstallLocation;
        /// User or Machine.
        PackageInstallScope PackageInstallScope;
        /// Silent, Interactive, or Default
        PackageInstallMode PackageInstallMode;
        /// Directs the logging to a log file. If provided, the installer must have write access to the file
        String LogOutputPath;
        /// Continues the install even if the hash in the catalog does not match the linked installer.
        Boolean AllowHashMismatch;
        /// Allows Store installs when Store Client is disabled.
        Boolean BypassIsStoreClientBlockedPolicyCheck;
        /// A string that will be passed to the installer.
        /// IMPLEMENTATION NOTE: maps to "--override" in the winget cmd line
        String ReplacementInstallerArguments;

        /// Used by a caller to correlate the install with a caller's data.
        /// The string must be JSON encoded.
        String CorrelationData;
        /// A string that will be passed to the source server if using a REST source
        String AdditionalPackageCatalogArguments;
    }

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageManager
    {
        PackageManager();

        /// Get the available catalogs. Each source will have a separate catalog.
        /// This does not open the catalog. These catalogs can be used individually or merged with CreateCompositePackageCatalogAsync.
        /// IMPLEMENTATION NOTE: This is a list of sources returned by Windows Package Manager source list
        Windows.Foundation.Collections.IVectorView<PackageCatalogReference> GetPackageCatalogs();
        /// Get a built in catalog
        PackageCatalogReference GetPredefinedPackageCatalog(PredefinedPackageCatalog predefinedPackageCatalog);
        /// Get a built in catalog
        PackageCatalogReference GetLocalPackageCatalog(LocalPackageCatalog localPackageCatalog);
        /// Get a catalog by a known name
        PackageCatalogReference GetPackageCatalogByName(String catalogName);
        /// Get a composite catalog to allow searching a user defined or pre defined source and a local source
        /// (Installing, Installed) together at the same time.
        PackageCatalogReference CreateCompositePackageCatalog(CreateCompositePackageCatalogOptions options);

        /// Install the specified package
        Windows.Foundation.IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackageAsync(CatalogPackage package, InstallOptions options);
    }

    /// Force midl3 to generate vector marshalling info.
    declare
    {
        interface Windows.Foundation.Collections.IVector<PackageCatalog>;
        interface Windows.Foundation.Collections.IVectorView<PackageCatalog>;
        interface Windows.Foundation.Collections.IVector<PackageCatalogInfo>;
        interface Windows.Foundation.Collections.IVectorView<PackageCatalogInfo>;
        interface Windows.Foundation.Collections.IVector<PackageCatalogReference>;
        interface Windows.Foundation.Collections.IVectorView<PackageCatalogReference>;
        interface Windows.Foundation.Collections.IVector<CatalogPackage>;
        interface Windows.Foundation.Collections.IVectorView<CatalogPackage>;
        interface Windows.Foundation.Collections.IVector<FindPackagesOptions>;
        interface Windows.Foundation.Collections.IVectorView<FindPackagesOptions>;
        interface Windows.Foundation.Collections.IVector<FindPackagesResult>;
        interface Windows.Foundation.Collections.IVectorView<FindPackagesResult>;
        interface Windows.Foundation.Collections.IVector<CreateCompositePackageCatalogOptions>;
        interface Windows.Foundation.Collections.IVectorView<CreateCompositePackageCatalogOptions>;
        interface Windows.Foundation.Collections.IVector<InstallOptions>;
        interface Windows.Foundation.Collections.IVectorView<InstallOptions>;
        interface Windows.Foundation.Collections.IVector<InstallResult>;
        interface Windows.Foundation.Collections.IVectorView<InstallResult>;
        interface Windows.Foundation.Collections.IVector<MatchResult>;
        interface Windows.Foundation.Collections.IVectorView<MatchResult>;
        interface Windows.Foundation.Collections.IVector<PackageManager>;
        interface Windows.Foundation.Collections.IVectorView<PackageManager>;
        interface Windows.Foundation.Collections.IVector<PackageMatchFilter>;
        interface Windows.Foundation.Collections.IVectorView<PackageMatchFilter>;
        interface Windows.Foundation.Collections.IVector<PackageVersionId>;
        interface Windows.Foundation.Collections.IVectorView<PackageVersionId>;
        interface Windows.Foundation.Collections.IVector<PackageVersionInfo>;
        interface Windows.Foundation.Collections.IVectorView<PackageVersionInfo>;
    }
}
```

# Appendix
