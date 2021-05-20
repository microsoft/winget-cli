# 1. Background

The Windows Package Manager currently exposes a command line interface to search for packages,
install them, view progress, and more. This API is designed to provide another way for callers to
make use of that functionality. The API will be preferred by callers that want to receive progress
and completion events, and UWP packages that do not have permission to launch command line processes.
The goal for this api is to provide the full set of install functionality possible using the Windows
Package Manager command line. The command line is documented at
https://docs.microsoft.com/en-us/windows/package-manager/winget/

# 2. Description

Windows Package Manager is a package manager for windows applications. It comes with a predefined
repository of applications and users can add new repositories using the winget command line. This
API allows packaged apps with the packageManagement capability and other higher privilege processes
to start, manage, and monitor installation of packages that are listed in Windows Package Manager
repositories.

# 3. Examples

Sample member values for the following examples:
m_installAppId = L"Microsoft.VSCode";

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

    IAsyncOperation<CatalogPackage> FindPackageInCatalog(AppCatalog catalog, std::wstring packageId)
    {
        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter = CreatePackageMatchFilter();
        filter.Field(PackageMatchField::Id);
        filter.Type(MatchType::Exact);
        filter.Value(packageId);
        findPackagesOptions.Selectors().Append(filter);
        FindPackagesResult findPackagesResult{ co_await catalog.FindPackagesAsync(findPackagesOptions) };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        co_return matches.GetAt(0).CatalogPackage();
    }

    IAsyncOperation<CatalogPackage> MainPage::FindPackage()
    {
        co_await winrt::resume_background();

        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalogReference catalogRef{ appInstaller.GetPredefinedAppCatalog(PredefinedAppCatalog::OpenWindowsCatalog) };
        AppCatalog catalog = catalogRef.ConnectAsync().get().AppCatalog();
        co_return FindPackageInCatalog(catalog, m_installAppId).get();
    }
```

## 3.3. Install

```c++ (C++ish pseudocode)

    IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackage(CatalogPackage package)
    {
        AppInstaller appInstaller = CreateAppInstaller();
        InstallOptions installOptions = CreateInstallOptions();
        installOptions.AppInstallScope(AppInstallScope::User);
        installOptions.AppInstallMode(AppInstallMode::Silent);

        return appInstaller.InstallPackageAsync(package, installOptions);
    }

    IAsyncAction UpdateUIProgress(
        InstallProgress progress, 
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar, 
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        co_await winrt::resume_foreground(progressBar.Dispatcher());
        progressBar.Value(progress.DownloadProgress);

        std::wstring downloadText{ L"Downloading. " };
        switch (progress.State)
        {
        case AppInstallProgressState::Queued:
            statusText.Text(L"Queued");
            break;
        case AppInstallProgressState::Downloading:
            downloadText += std::to_wstring(progress.BytesDownloaded) + L" bytes of " + std::to_wstring(progress.BytesRequired);
            statusText.Text(downloadText.c_str());
            break;
        case AppInstallProgressState::Installing:
            statusText.Text(L"Installing");
            progressBar.IsIndeterminate(true);
            break;
        case AppInstallProgressState::PostInstall:
            statusText.Text(L"Finishing install");
            break;
        case AppInstallProgressState::Finished:
            statusText.Text(L"Finished install.");
            progressBar.IsIndeterminate(false);
            break;
        default:
            statusText.Text(L"");
        }
    }

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
        catch (winrt::hresult_error const& ex)
        {
            // Operation failed
            // Example: HRESULT_FROM_WIN32(ERROR_DISK_FULL).
            installOperationHr = ex.code();
            // Example: "There is not enough space on the disk."
            errorMessage = ex.message();
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
            statusText.Text(errorMessage.c_str());
        }
        else if (installResult.RebootRequired())
        {
            installButton.Content(box_value(L"Install"));
            statusText.Text(L"Reboot to finish installation.");
        }
        else
        {
            installButton.Content(box_value(L"Install"));
            statusText.Text(L"Finished.");
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

        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalogReference catalogRef{ appInstaller.GetPredefinedAppCatalog(PredefinedAppCatalog::OpenWindowsCatalog) };
        AppCatalog catalog = catalogRef.Connect().AppCatalog();
        if (!catalog)
        {
            co_await winrt::resume_foreground(progressBar.Dispatcher());
            statusText.Text(L"Connecting to catalog failed.");
            co_return;
        }
        FindPackagesResult findPackagesResult{ TryFindPackageInCatalog(catalog, m_installAppId).get() };

        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        if (matches.Size() > 0)
        {
            m_installPackageOperation = InstallPackage(matches.GetAt(0).CatalogPackage());
            UpdateUIForInstall(m_installPackageOperation, installButton, cancelButton, progressBar, statusText);
        }
    }

```

## 3.4.1 Cancel

The async operation must be stored, or the install code must wait on an event that can be triggered.

```c++ (C++ish pseudocode)
    void MainPage::CancelButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation)
        {
            m_installPackageOperation.Cancel();
        }
    }
```

## 3.4.2. Cancel

Cancel the async operation

```c++ (C++ish pseudocode)
    IAsyncAction CancelInstall(
        std::wstring installAppId)
    {
        co_await winrt::resume_background();
        // Creation of the AppInstaller has to use CoCreateInstance rather than normal winrt initialization since it's created 
        // by an out of proc com server.
        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalogReference catalogRef{ appInstaller.GetPredefinedAppCatalog(PredefinedAppCatalog::OpenWindowsCatalog) };
        AppCatalog windowsCatalog = catalogRef.Connect().AppCatalog();
        if (!windowsCatalog)
        {
            co_return;
        }
        AppCatalogReference installedCatalogRef{ appInstaller.GetLocalAppCatalog(LocalAppCatalog::InstallingPackages) };
        AppCatalog installedCatalog = installedCatalogRef.Connect().AppCatalog();
        if (!installedCatalog)
        {
            co_return;
        }
        CreateCompositeAppCatalogOptions createCompositeAppCatalogOptions = CreateCreateCompositeAppCatalogOptions();
        createCompositeAppCatalogOptions.Catalogs().Append(windowsCatalog);
        createCompositeAppCatalogOptions.LocalAppCatalog(installedCatalog);
        // Specify that the search behavior is to only query for local packages.
        // Since the local catalog that is open is InstallingPackages, this will only find a result if installAppId is 
        // currently installing.
        createCompositeAppCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::InstallingPackages);
        AppCatalogReference compositeCatalogRef{ appInstaller.CreateCompositeAppCatalog(createCompositeAppCatalogOptions) };
        AppCatalog compositeCatalog = compositeCatalogRef.Connect().AppCatalog();
        if (!compositeCatalog)
        {
            co_return;
        }

        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter;
        filter.Field(PackageMatchField::Id);
        filter.Type(MatchType::Exact);
        filter.Value(installAppId);
        findPackagesOptions.Selectors().Append(filter);
        FindPackagesResult findPackagesResult{ compositeCatalog.FindPackagesAsync(findPackagesOptions).get() };
        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        CatalogPackage package = matches.GetAt(0).CatalogPackage();

        if (package.IsInstalling())
        {
            Windows::Foundation::IAsyncOperationWithProgress<InstallResult, InstallProgress> installOperation = appInstaller.GetInstallProgress(package);
            installOperation.Cancel();
        }
    }


```

## 3.5. Get progress for installing app

Check which packages are installing and show progress. This can be useful if the calling app closes
and reopens while an install is still in progress.

```c++ (C++ish pseudocode)

    MainPage::MainPage()
    {
        InitializeComponent();
        m_installAppId = L"Microsoft.VSCode";
        InitializeInstallUI(m_installAppId, installButton(), installProgressBar(), installStatusText());
    }

    IAsyncAction MainPage::InitializeInstallUI(
        std::wstring installAppId,
        winrt::Windows::UI::Xaml::Controls::Button installButton,
        winrt::Windows::UI::Xaml::Controls::Button cancelButton,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        co_await winrt::resume_background();
        // Creation of the AppInstaller has to use CoCreateInstance rather than normal winrt initialization since it's created 
        // by an out of proc com server.
        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalogReference catalogRef{ appInstaller.GetPredefinedAppCatalog(PredefinedAppCatalog::OpenWindowsCatalog) };
        AppCatalog windowsCatalog = catalogRef.ConnectAsync().get().AppCatalog();
        if (!windowsCatalog)
        {
            co_return;
        }

        AppCatalogReference installedCatalogRef{ appInstaller.GetLocalAppCatalog(LocalAppCatalog::InstallingPackages) };
        AppCatalog installedCatalog = installedCatalogRef.ConnectAsync().get().AppCatalog();
        if (!installedCatalog)
        {
            // A local catalog failing would be very unexpected, but checking for null as best practice.
            co_return;
        }
        // Get a composite catalog that allows search of both the OpenWindowsCatalog and InstallingPackages.
        // Creation of the AppInstaller has to use CoCreateInstance rather than normal winrt initialization since it's created 
        // by an out of proc com server.
        CreateCompositeAppCatalogOptions createCompositeAppCatalogOptions = CreateCreateCompositeAppCatalogOptions();
        createCompositeAppCatalogOptions.Catalogs().Append(windowsCatalog);
        createCompositeAppCatalogOptions.LocalAppCatalog(installedCatalog);
        // Specify that the search behavior is to only query for local packages.
        // Since the local catalog that is open is InstallingPackages, this will only find a result if installAppId is 
        // currently installing.
        createCompositeAppCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::AllLocalPackages);
        AppCatalogReference compositeCatalogRef{ appInstaller.CreateCompositeAppCatalog(createCompositeAppCatalogOptions) };
        ConnectResult connectResult{ co_await compositeCatalogRef.ConnectAsync() };
        AppCatalog compositeCatalog = connectResult.AppCatalog();

        FindPackagesOptions findPackagesOptions = CreateFindPackagesOptions();
        PackageMatchFilter filter;
        filter.Field(PackageMatchField::Id);
        filter.Type(MatchType::Exact);
        filter.Value(installAppId);
        findPackagesOptions.Selectors().Append(filter);
        FindPackagesResult findPackagesResult{ compositeCatalog.FindPackagesAsync(findPackagesOptions).get() };
        winrt::IVectorView<MatchResult> matches = findPackagesResult.Matches();
        CatalogPackage package = matches.GetAt(0).CatalogPackage();

        if (package.IsInstalling())
        {
            m_installPackageOperation = appInstaller.GetInstallProgress(package);
            UpdateUIForInstall(m_installPackageOperation, installButton, cancelButton, progressBar, statusText);
        }
    }
```

## 3.6. Open a catalog by name

Open a catalog known to the caller. There is no way to use the api to add a catalog, that must be done
on the command line.

```c++ (C++ish pseudocode)
    IAsyncOperation<AppCatalog> FindSource(std::wstring packageSource)
    {
        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalogReference catalogRef{ appInstaller.GetAppCatalogById(packageSource) };
        if (catalogRef)
        {
            ConnectResult connectResult{ co_await catalogRef.ConnectAsync() };
            // AppCatalog will be null if connectResult.ErrorCode() is a failure
            AppCatalog catalog = connectResult.AppCatalog();
            co_return catalog;
        }
    }
```

# 4 Remarks

Notes have been added inline throughout the api details.

The biggest open question right now is whether the api needs to provide a way for the client to verify
that the server is actually the Windows Package Manager package.
The concern is the following scenario:
The client app has packageManagement capability which currently only allows install of msix applications.
The user clicks on buttons in the client app to start an install of Microsoft.VSCode. The client app calls
the server to do the install, and a UAC prompt is shown.
The UAC prompt indicates that the app asking for permission is not Microsoft.VSCode, but rather some other
app, however the user does not bother to read the prompt and simply clicks accept.
In this way, an app that takes over the com registration of the Windows Package Manager could use the user's
mistake to allow itself to escalate.
A proposed mitigation is provided in OpenCatalogAsync which is an optional call.

Another question is whether it's required to provide a wrapper api for this api to avoid callers having to
use CoCreateInstance, or write their own wrapper to project into c#.
We could provide a 1 to 1 wrapper implementation in Microsoft.Management.Deployment.Client.

Naming of the items is as always a tricky question. For this api there are multiple similar apis that are
relevant with regard to naming and consistency. There is the Windows Package Manager command line which uses
"source" to describe the various repositories that can host packages and "search" to describe looking up an app.
https://docs.microsoft.com/en-us/windows/package-manager/winget/
There is the Windows::ApplicationModel::PackageCatalog which exists as a Windows API for installing packages
and monitoring their installation progress.
https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.packagecatalog?view=winrt-19041
And there is Windows.Management.Deployment.PackageManager which allows packages with the packageManagement
capability to install msix apps and uses "Find" to describe looking up an app
https://docs.microsoft.com/en-us/uwp/api/windows.management.deployment.packagemanager?view=winrt-19041

I've chosen to align with the Windows APIs in using \*Catalog and Find. But this may cause some confusion if
callers are using both the winget command line and this API. In particular a problem may be that since this
API does not yet propose to implement adding a "source", client applications would need to work with the
command line interface in order to do that which may cause the name change from Source to AppCatalog to be
particularly confusing.

# 5 API Details

```c# (but really MIDL3)
namespace Microsoft.Management.Deployment
{
    [contractversion(1)]
    apicontract WindowsPackageManagerContract{};

    /// State of the install.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum AppInstallProgressState
    {
        /// The install is queued but not yet active. Cancellation of the IAsyncOperationWithProgress in this 
        /// state will prevent the app from downloading or installing.
        Queued,
        /// The installer is downloading. Cancellation of the IAsyncOperationWithProgress in this state will 
        /// end the download and prevent the app from installing.
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
        AppInstallProgressState State;
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

    /// Result of the install
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass InstallResult
    {
        /// Used by a caller to correlate the install with a caller's data.
        String CorrelationData{ get; };
        /// Whether a restart is required to complete the install.
        Boolean RebootRequired{ get; };

        /// Batched error code, example APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED
        HRESULT ErrorCode{ get; };
        /// Specific error if known, from downloader or installer itself, example ERROR_INSTALL_PACKAGE_REJECTED
        HRESULT ExtendedErrorCode{ get; };
    }

    /// IMPLEMENTATION NOTE: SourceOrigin from AppInstallerRepositorySource.h
    /// Defines the origin of the app catalog details.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum AppCatalogOrigin
    {
        /// Predefined means it came as part of the Windows Package Manager package and cannot be removed.
        Predefined,
        /// User means it was added by the user and could be removed.
        User,
    };

    /// IMPLEMENTATION NOTE: SourceTrustLevel from AppInstallerRepositorySource.h
    /// Defines the trust level of the app catalog.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum AppCatalogTrustLevel
    {
        None,
        Trusted,
    };

    /// IMPLEMENTATION NOTE: SourceDetails from AppInstallerRepositorySource.h
    /// Interface for retrieving information about an app catalog without acting on it.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass AppCatalogInfo
    {
        /// The app catalog's unique identifier. 
        /// SAMPLE VALUES: For OpenWindowsCatalog "Microsoft.Winget.Source_8wekyb3d8bbwe"
        /// For contoso sample on msdn "contoso"
        String Id { get; };
        /// The name of the app catalog. 
        /// SAMPLE VALUES: For OpenWindowsCatalog "winget".
        /// For contoso sample on msdn "contoso"
        String Name { get; };
        /// The type of the app catalog.
        /// ALLOWED VALUES: "Microsoft.Rest", "Microsoft.PreIndexed.Package"
        /// SAMPLE VALUES: For OpenWindowsCatalog "Microsoft.PreIndexed.Package".
        /// For contoso sample on msdn "Microsoft.PreIndexed.Package"
        String Type { get; };
        /// The argument used when adding the app catalog.
        /// SAMPLE VALUES: For OpenWindowsCatalog "https://winget.azureedge.net/cache"
        /// For contoso sample on msdn "https://pkgmgr-int.azureedge.net/cache"
        String Argument { get; };
        /// The app catalog's extra data string which was passed in when adding the source.
        /// SAMPLE VALUES: For OpenWindowsCatalog "Microsoft.Winget.Source_8wekyb3d8bbwe"
        /// For contoso sample on msdn "contoso"
        String ExtraData { get; };
        /// The last time that this app catalog was updated.
        Windows.Foundation.DateTime LastUpdateTime { get; };
        /// The origin of the app catalog.
        AppCatalogOrigin Origin { get; };
        /// The trust level of the app catalog
        AppCatalogTrustLevel TrustLevel { get; };
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
        Publisher,
        /// The locale of the package
        Locale,
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
        String Name { get; };
        String Version { get; };
        String Channel { get; };
        /// DESIGN NOTE: RelativePath from AppInstallerRepositorySearch.h is excluded as not needed.
        /// String RelativePath;

        /// IMPLEMENTATION NOTE: PackageVersionMultiProperty fields from AppInstallerRepositorySearch.h
        /// PackageFamilyName and ProductCode can have multiple values.
        Windows.Foundation.Collections.IVectorView<String> PackageFamilyNames { get; };
        Windows.Foundation.Collections.IVectorView<String> ProductCodes { get; };

        /// Gets the app catalog  where this package version is from.
        AppCatalogReference AppCatalogReference { get; };

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
        /// The app catalog id that this version came from.
        String AppCatalogId { get; };
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
        /// IMPLEMENTATION NOTE: This would be new work as Windows Package Manager does not currently create a 
        /// CatalogPackage object from a manifest.
        /// DESIGN NOTE: This is proposed as the preferred alternative to having InstallOptions match the winget
        /// command line which allows you to pass in a manifestPath directly into install. Allowing creation of 
        /// a CatalogPackage object from a manifest allows the install api to consistently use package objects 
        /// rather than having a "you can either set the manifest path or use a package but not both" problem.
        static CatalogPackage TryCreateFromManifestPath(String manifestPath);

        /// IMPLEMENTATION NOTE: PackageProperty fields from AppInstallerRepositorySearch.h
        /// Gets a property of this package.
        String Id { get; };
        String Name { get; };

        /// Gets the installed package information if the package is installed.
        PackageVersionInfo InstalledVersion{ get; };

        /// Gets all available versions of this package.
        /// The versions will be returned in sorted, descending order.
        ///  Ex. { 4, 3, 2, 1 }
        Windows.Foundation.Collections.IVectorView<PackageVersionId> AvailableVersions { get; };

        /// Gets the latest version of this package.
        PackageVersionInfo LatestAvailableVersion { get; };

        /// Gets a specific version of this package.
        PackageVersionInfo GetAvailableVersion(PackageVersionId versionKey);

        /// Gets a value indicating whether an available version is newer than the installed version.
        Boolean IsUpdateAvailable { get; };

        /// Gets whether the package is installing
        Boolean IsInstalling { get; };

        /// DESIGN NOTE:
        /// IsSame from IPackage in AppInstallerRepositorySearch is not implemented in V1.
        /// Determines if the given IPackage refers to the same package as this one.
        /// virtual bool IsSame(const IPackage*) const = 0;
    }

    /// IMPLEMENTATION NOTE: CompositeSearchBehavior from AppInstallerRepositorySource.h
    /// Search behavior for composite sources.
    /// Only relevant for composite sources with a local source, not for aggregates of multiple available sources.
    /// Installed and available packages in the result are always correlated when possible.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum CompositeSearchBehavior
    {
        /// Search only installed packages
        InstalledPackages,
        /// Search only installing packages
        InstallingPackages,
        /// Search only local packages (installed and installing)
        AllLocalPackages,
        /// Search both local and remote sources.
        AllPackages,
    };

    /// IMPLEMENTATION NOTE: MatchType from AppInstallerRepositorySearch.h
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum MatchType
    {
        Exact,
        CaseInsensitive,
        StartsWith,
        Fuzzy,
        Substring,
        FuzzySubstring,
        Wildcard,
    };

    /// IMPLEMENTATION NOTE: PackageMatchField from AppInstallerRepositorySearch.h
    /// The field to match on.
    /// The values must be declared in order of preference in search results.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PackageMatchField
    {
        AppCatalogDefined,
        Id,
        Name,
        Moniker,
        Command,
        Tag,
        /// DESIGN NOTE: The following PackageMatchField from AppInstallerRepositorySearch.h are not implemented in V1.
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
        MatchType Type;
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

    /// IMPLEMENTATION NOTE: SearchResult from AppInstallerRepositorySearch.h
    /// Search result data returned from FindPackages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass FindPackagesResult
    {
        /// Error codes
        HRESULT ErrorCode{ get; };

        /// The full set of results from the search.
        Windows.Foundation.Collections.IVectorView<MatchResult> Matches { get; };

        /// If true, the results were truncated by the given ResultLimit
        /// USAGE NOTE: Windows Package Manager does not support result pagination, there is no way to continue 
        /// getting more results.
        Boolean IsTruncated{ get; };
    }

    /// Options for FindPackages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass FindPackagesOptions
    {
        FindPackagesOptions();

        /// DESIGN NOTE: 
        /// This class maps to SearchRequest from  AppInstallerRepositorySearch.h 
        /// That class is a container for data used to filter the available manifests in an app catalog.
        /// Its properties can be thought of as:
        /// (Query || Inclusions...) && Filters...
        /// If Query and Inclusions are both empty, the starting data set will be the entire database.
        /// Everything && Filters...
        /// That has been translated in this api so that 
        /// Inclusions are Selectors below
        /// Filters are Filters below
        /// Query is PackageMatchField::AppCatalogDefined and in the Selector list.
        /// USAGE NOTE: Only one selector with PackageMatchField::AppCatalogDefined is allowed.

        /// Selectors = you have to match at least one selector (if there are no selectors, then nothing is selected)
        Windows.Foundation.Collections.IVector<PackageMatchFilter> Selectors { get; };
        /// Filters = you have to match all filters(if there are no filters, then there is no filtering of selected items)
        Windows.Foundation.Collections.IVector<PackageMatchFilter> Filters{ get; };

        /// Restricts the length of the returned results to the specified count.
        UInt32 ResultLimit;

        /// Overrides the default search behavior for this search if the catalog is a composite catalog.
        /// IMPLEMENTATION NOTE: Windows Package Manager currently only supports setting the search behavior when 
        /// creating the source so this is new work but is expected to be very small.
        CompositeSearchBehavior CompositeSearchBehavior;
    }

    /// IMPLEMENTATION NOTE: ISource from AppInstallerRepositorySource.h
    /// A catalog for searching for packages
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass AppCatalog
    {
        /// Gets a value indicating whether this app catalog is a composite of other app catalogs,
        /// and thus the packages may come from disparate app catalogs as well.
        Boolean IsComposite { get; };
        /// The details of the app catalog if it is not a composite.
        AppCatalogInfo Info { get; };

        /// Searches for Packages in the catalog.
        Windows.Foundation.IAsyncOperation<FindPackagesResult> FindPackagesAsync(FindPackagesOptions options);
        FindPackagesResult FindPackages(FindPackagesOptions options);
    }

    /// Result of the Connect call
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass ConnectResult
    {
        /// Error codes
        HRESULT ErrorCode{ get; };

        AppCatalog AppCatalog { get; };
    }

    /// A reference to a catalog that callers can try to Connect.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass AppCatalogReference
    {
        /// The details of the app catalog if it is not a composite.
        AppCatalogInfo Info { get; };

        /// Opens a catalog. Required before searching. For remote catalogs (i.e. not Installed and Installing) this 
        /// may require downloading information from a server.
        Windows.Foundation.IAsyncOperation<ConnectResult> ConnectAsync();
        ConnectResult Connect();
    }

    /// Catalogs with AppCatalogOrigin Predefined
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum PredefinedAppCatalog
    {
        OpenWindowsCatalog,
    };

    /// Local Catalogs with AppCatalogOrigin Predefined
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum LocalAppCatalog
    {
        InstalledPackages,
        /// IMPLEMENTATION NOTE:  Windows Package Manager does not currently have this concept. 
        /// This would be a new Orchestration feature to keep track of the installing CatalogPackage objects and allow 
        /// search of those objects.
        InstallingPackages,
        AllLocalPackages
    };

    /// 
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass CreateCompositeAppCatalogOptions
    {
        CreateCompositeAppCatalogOptions();

        /// Create a composite catalog to allow searching a user defined or pre defined source
        /// and a local source (Installing, Installed) together  
        /// IMPLEMENTATION NOTE: Windows Package Manager currently only supports 
        /// one local source per composite catalog.
        /// DESIGN NOTE: It seems likely that in the future we will want to support 
        /// letting callers use two local sources (i.e. both Installing and Installed)
        IVector<AppCatalog> Catalogs { get; };
        /// The local source
        AppCatalog LocalAppCatalog;
        /// Sets the default search behavior if the catalog is a composite catalog.
        CompositeSearchBehavior CompositeSearchBehavior;
    }

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum AppInstallScope
    {
        User,
        Machine,
    };

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    enum AppInstallMode
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
        AppInstallScope AppInstallScope;
        /// Silent, Interactive, or Default
        AppInstallMode AppInstallMode;
        /// Directs the logging to a log file. Callers must provide a path to a file that the Windows Package 
        /// Manager package has write access to.
        String LogOutputPath;
        /// Continues the install even if the hash in the catalog does not match the linked installer.
        Boolean AllowHashMismatch;
        /// A string that will be passed to the installer. 
        /// IMPLEMENTATION NOTE: maps to "--override" in the winget cmd line
        String ReplacementInstallerArguments;

        /// Used by a caller to correlate the install with a caller's data.
        /// IMPLEMENTATION NOTE: This area is in flux but right now it's expected that this would allow a caller to 
        /// use JSON to encode multiple arguments.
        String CorrelationData;
        /// A string that will be passed to the source server if using a REST source
        String AdditionalAppCatalogArguments;
    }

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass AppInstaller
    {
        AppInstaller();

        /// Get the available catalogs. Each source will have a separate catalog. This list does not include KnownCatalogs.
        /// This does not open the catalog. These catalogs can be used individually or merged with CreateCompositeAppCatalogAsync. 
        /// IMPLEMENTATION NOTE: This is a list of sources returned by Windows Package Manager source list 
        /// excluding the predefined sources.
        Windows.Foundation.Collections.IVectorView<AppCatalogReference> GetAppCatalogs();
        /// Get a built in catalog
        AppCatalogReference GetPredefinedAppCatalog(PredefinedAppCatalog predefinedAppCatalog);
        /// Get a built in catalog
        AppCatalogReference GetLocalAppCatalog(LocalAppCatalog localAppCatalog);
        /// Get a catalog by a known unique identifier
        AppCatalogReference GetAppCatalogById(String catalogId);
        /// Get a composite catalog to allow searching a user defined or pre defined source and a local source
        /// (Installing, Installed) together at the same time.
        AppCatalogReference CreateCompositeAppCatalog(CreateCompositeAppCatalogOptions options);

        /// Install the specified package
        Windows.Foundation.IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackageAsync(CatalogPackage package, InstallOptions options);
        /// Get install progress
        Windows.Foundation.IAsyncOperationWithProgress<InstallResult, InstallProgress> GetInstallProgress(CatalogPackage package);
    }

    /// Force midl3 to generate vector marshalling info. 
    declare
    {
        interface Windows.Foundation.Collections.IVector<AppCatalog>;
        interface Windows.Foundation.Collections.IVectorView<AppCatalog>;
        interface Windows.Foundation.Collections.IVector<AppCatalogInfo>;
        interface Windows.Foundation.Collections.IVectorView<AppCatalogInfo>;
        interface Windows.Foundation.Collections.IVector<AppCatalogReference>;
        interface Windows.Foundation.Collections.IVectorView<AppCatalogReference>;
        interface Windows.Foundation.Collections.IVector<CatalogPackage>;
        interface Windows.Foundation.Collections.IVectorView<CatalogPackage>;
        interface Windows.Foundation.Collections.IVector<FindPackagesOptions>;
        interface Windows.Foundation.Collections.IVectorView<FindPackagesOptions>;
        interface Windows.Foundation.Collections.IVector<FindPackagesResult>;
        interface Windows.Foundation.Collections.IVectorView<FindPackagesResult>;
        interface Windows.Foundation.Collections.IVector<CreateCompositeAppCatalogOptions>;
        interface Windows.Foundation.Collections.IVectorView<CreateCompositeAppCatalogOptions>;
        interface Windows.Foundation.Collections.IVector<InstallOptions>;
        interface Windows.Foundation.Collections.IVectorView<InstallOptions>;
        interface Windows.Foundation.Collections.IVector<InstallResult>;
        interface Windows.Foundation.Collections.IVectorView<InstallResult>;
        interface Windows.Foundation.Collections.IVector<MatchResult>;
        interface Windows.Foundation.Collections.IVectorView<MatchResult>;
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
