
# 1. Background

The Windows Package Manager currently exposes a command line interface to search for packages, install them, view progress, and more. 
This API is designed to provide another way for callers to make use of that functionality. The API will be preferred by 
callers that want to recieve progress and completion events, and UWP packages that do not have permission to launch command line 
processes. The goal for this api is to provide the full set of install functionality possible using the Windows Package Manager command line.
The command line is documented at https://docs.microsoft.com/en-us/windows/package-manager/winget/

# 2. Description

Windows Package Manager is a package manager for windows applications. It comes with a predefined repository of applications and users can add 
new repositories using the winget command line. This API allows packaged apps with the packageManagement capability and other higher 
privilege processes to start, manage, and monitor installation of packages that are listed in Windows Package Manager repositories.

# 3. Examples

Sample member values for the following examples:

```c++ (C++ish pseudocode)
m_installAppId = L"Microsoft.VSCode";
```

## 3.1. Create objects

Creation of objects has to be done through CoCreateInstance rather than normal winrt initialization since it's hosted by an out of proc com server.
These helper methods will be used in the rest of the examples.

```c++ (C++ish pseudocode)
    AppInstaller CreateAppInstaller()
    {
        com_ptr<abi::IAppInstaller> abiAppInstaller = nullptr;
        check_hresult(CoCreateInstance(CLSID_AppInstaller, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(abiAppInstaller.put())));
        check_pointer(abiAppInstaller.get());
        AppInstaller appInstaller{ abiAppInstaller.as<AppInstaller>() };
        return appInstaller;
    }
    InstallOptions CreateInstallOptions()
    {
        com_ptr<abi::IInstallOptions> abiInstallOptions = nullptr;
        check_hresult(CoCreateInstance(CLSID_InstallOptions, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(abiInstallOptions.put())));
        check_pointer(abiInstallOptions.get());
        InstallOptions installOptions{ abiInstallOptions.as<winrt::Microsoft::Management::Deployment::InstallOptions>() };
        return installOptions;
    }
    FindPackagesOptions CreateFindOptions()
    {
        com_ptr<abi::IFindPackagesOptions> abifindOptions = nullptr;
        check_hresult(CoCreateInstance(CLSID_FindPackagesOptions, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(abifindOptions.put())));
        check_pointer(abifindOptions.get());
        FindPackagesOptions findPackagesOptions{ abifindOptions.as<FindPackagesOptions>() };
        return findPackagesOptions;
    } 
    GetCompositeAppCatalogOptions CreateGetCompositeAppCatalogOptions()
    {
        com_ptr<abi::IGetCompositeAppCatalogOptions> abiGetCompositeAppCatalogOptions = nullptr;
        check_hresult(CoCreateInstance(CLSID_GetCompositeAppCatalogOptions), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(abiGetCompositeAppCatalogOptions.put())));
        check_pointer(abiGetCompositeAppCatalogOptions.get());
        GetCompositeAppCatalogOptions getCompositeAppCatalogOptions{ abiGetCompositeAppCatalogOptions.as<GetCompositeAppCatalogOptions>() };
        return getCompositeAppCatalogOptions;
    }
```

## 3.2. Search

The api can be used to search for packages in a catalog known to Windows Package Manager. This can be used to get availability information
or start an install.

```c++ (C++ish pseudocode)


    IAsyncOperation<Package> FindPackageInCatalog(AppCatalog catalog, std::wstring packageId)
    {
        FindPackagesOptions findPackagesOptions = CreateFindOptions();
        PackageMatchFilter filter;
        filter.IsAdditive = true;
        filter.Field = PackageMatchField::Id;
        filter.Type = MatchType::Exact;
        filter.Value = packageId;
        findPackagesOptions.Filters().Append(filter);
        FindPackagesResult findPackagesResult{ co_await catalog.FindPackagesAsync(findPackagesOptions) };

        winrt::IVectorView<ResultMatch> matches = findPackagesResult.Matches();
        co_return matches.GetAt(0).Package();
    }

    IAsyncOperation<Package> MainPage::FindPackage()
    {
        // Capture the ui thread context.
        winrt::apartment_context uiThread;
        co_await winrt::resume_background();

        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalog catalog{ co_await appInstaller.GetAppCatalogAsync(PredefinedAppCatalog::OpenWindowsCatalog) };
        co_await catalog.OpenAsync();
        Package package = { co_await FindPackageInCatalog(catalog, m_installAppId) };

        co_return package;
    }

```

## 3.3. Install

```c++ (C++ish pseudocode)

    IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackage(Package package)
    {
        AppInstaller appInstaller = CreateAppInstaller();
        InstallOptions installOptions = CreateInstallOptions();

        installOptions.InstallScope(InstallScope::User);
        installOptions.Package(package);
        installOptions.InstallMode(InstallMode::Silent);

        return appInstaller.InstallPackageAsync(installOptions);
    }

    IAsyncAction UpdateUIProgress(InstallProgress progress, winrt::apartment_context uiThread, winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar, winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        co_await uiThread;
        progressBar.Value(progress.DownloadPercentage);

        std::wstring downloadText{ L"Downloading." };
        switch (progress.State)
        {
        case InstallProgressState::Queued:
            statusText.Text(L"Queued");
            break;
        case InstallProgressState::Downloading:
            downloadText += progress.BytesDownloaded + L" bytes of " + progress.BytesRequired;
            statusText.Text(downloadText.c_str());
            break;
        case InstallProgressState::Installing:
            statusText.Text(L"Installing");
            break;
        case InstallProgressState::PostInstall:
            statusText.Text(L"Finishing install");
            break;
        default:
            statusText.Text(L"");
        }
        co_return;
    }

    IAsyncAction UpdateUIForInstall(
        IAsyncOperationWithProgress<InstallResult, InstallProgress> installPackageOperation, 
        winrt::apartment_context uiThread,
        winrt::Windows::UI::Xaml::Controls::Button button,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar, 
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        installPackageOperation.Progress([&](
            IAsyncOperationWithProgress<InstallResult, InstallProgress> const& /* sender */,
            InstallProgress const& progress)
        {
            UpdateUIProgress(progress, uiThread, progressBar, statusText).get();
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
        co_await uiThread;

        if (installPackageOperation.Status() == AsyncStatus::Canceled)
        {
            button.Content(box_value(L"Retry"));
            statusText.Text(L"Install cancelled.");
        }
        if (installPackageOperation.Status() == AsyncStatus::Error || installResult == nullptr)
        {
            button.Content(box_value(L"Error"));
            statusText.Text(errorMessage.c_str());
        }
        else if (installResult.RebootRequired())
        {
            button.Content(box_value(L"Reboot Required."));
            statusText.Text(L"Reboot to finish installation.");
        }
        else
        {
            button.Content(box_value(L"Installed"));
        }
    }

    IAsyncAction MainPage::StartInstall(
        winrt::Windows::UI::Xaml::Controls::Button button,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        // Capture the ui thread context.
        winrt::apartment_context uiThread;
        co_await winrt::resume_background();

        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalog catalog{ co_await appInstaller.GetAppCatalogAsync(PredefinedAppCatalog::OpenWindowsCatalog) };
        co_await catalog.OpenAsync();
        Package package{ co_await FindPackageInCatalog(catalog, m_installAppId) };

        m_installPackageOperation = InstallPackage(package);
        UpdateUIForInstall(m_installPackageOperation, uiThread, button, progressBar, statusText);
        co_return;
    }

    void MainPage::InstallButtonClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_installPackageOperation == nullptr)
        {
            StartInstall(installButton(), installProgressBar(), installStatusText());
        }
    }
```

## 3.4. Cancel

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

## 3.5. Get progress for installing app

Check which packages are installing and show progress. This can be useful if the calling app closes and reopens while an install is still in progress.

```c++ (C++ish pseudocode)

    MainPage::MainPage()
    {
        InitializeComponent();
        m_installAppId = L"Microsoft.VSCode";
        InitializeInstallUI(m_installAppId, installButton(), installProgressBar(), installStatusText());
    }

    IAsyncAction MainPage::InitializeInstallUI(
        std::wstring installAppId,
        winrt::Windows::UI::Xaml::Controls::Button button,
        winrt::Windows::UI::Xaml::Controls::ProgressBar progressBar,
        winrt::Windows::UI::Xaml::Controls::TextBlock statusText)
    {
        winrt::apartment_context uiThread;

        co_await winrt::resume_background();
        // Creation of the AppInstaller has to use CoCreateInstance rather than normal winrt initialization since it's created by an out of proc com server.
        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalog windowsCatalog{ co_await appInstaller.GetAppCatalogAsync(PredefinedAppCatalog::OpenWindowsCatalog) };
        co_await windowsCatalog.OpenAsync();
        AppCatalog installingCatalog{ co_await appInstaller.GetAppCatalogAsync(PredefinedAppCatalog::InstallingPackages) };
        co_await installingCatalog.OpenAsync();
        Windows::Foundation::Collections::IVector<AppCatalog> catalogs{ winrt::single_threaded_vector<AppCatalog>() };
        // Get a composite catalog that allows search of both the OpenWindowsCatalog and InstallingPackages.
        // Creation of the GetCompositeAppCatalogOptions has to use CoCreateInstance rather than normal winrt initialization since it's created by an out of proc com server.
        GetCompositeAppCatalogOptions getCompositeAppCatalogOptions = CreateGetCompositeAppCatalogOptions();
        getCompositeAppCatalogOptions.Catalogs().Append(windowsCatalog);
        getCompositeAppCatalogOptions.Catalogs().Append(installingCatalog);
        // Specify that the search behavior is to only query for local packages.
        // Since the local catalog that is open is InstallingPackages, this will only find a result if installAppId is currently installing.
        getCompositeAppCatalogOptions.CompositeSearchBehavior(CompositeSearchBehavior::Installing);
        AppCatalog compositeCatalog{ co_await appInstaller.GetCompositeAppCatalogAsync(getCompositeAppCatalogOptions) };
        co_await compositeCatalog.OpenAsync();

        FindPackagesOptions findPackagesOptions = CreateFindOptions();
        PackageMatchFilter filter;
        filter.IsAdditive = true;
        filter.Field = PackageMatchField::Id;
        filter.Type = MatchType::Exact;
        filter.Value = installAppId;
        findPackagesOptions.Filters().Append(filter);
        FindPackagesResult findPackagesResult{ co_await compositeCatalog.FindPackagesAsync(findPackagesOptions) };
        winrt::IVectorView<ResultMatch> matches = findPackagesResult.Matches();
        Package package = matches.GetAt(0).Package();

        if (package.IsInstalling())
        {
            m_installPackageOperation = package.InstallProgress();
            UpdateUIForInstall(m_installPackageOperation, uiThread, button, progressBar, statusText); 
            // Alternatively to Cancel the operation, use:
            // m_installPackageOperation.Cancel();
        }
    }
```

## 3.6. Find sources

```c++ (C++ish pseudocode)
    IAsyncOperation<AppCatalog> FindDefaultSource()
    {
        AppInstaller appInstaller = CreateAppInstaller();

        winrt::IVectorView<AppCatalog> catalogs{ co_await appInstaller.GetAppCatalogsAsync() };
        for (AppCatalog catalog : catalogs)
        {
            if (catalog.Details().Origin == AppCatalogOrigin::Default)
            {
                co_await catalog.OpenAsync();
                co_return catalog;
            }
        }
        winrt::throw_hresult(E_UNEXPECTED);
    }
```

## 3.7. Open a catalog by name

Open a user-added catalog by name. There is no way to use the api to add a catalog, that must be done on the command line.

```c++ (C++ish pseudocode)

    IAsyncOperation<AppCatalog> FindSource(std::wstring packageSource)
    {
        AppInstaller appInstaller = CreateAppInstaller();
        AppCatalog catalog{ co_await appInstaller.GetAppCatalogAsync(packageSource) };
        co_await catalog.OpenAsync();
        co_return catalog;
    }
```

# 4 Remarks

Notes have been added inline throughout the api details.

The biggest open question right now is whether the api needs to provide a way for the client to verify that the server is actually the Windows Package Manager package.
The concern is the following scenario: 
The client app has packageManagement capability which currently only allows install of msix applications.
The user clicks on buttons in the client app to start an install of Microsoft.VSCode. The client app calls the server to do the install, and a UAC prompt is shown.
The UAC prompt indicates that the app asking for permission is not Microsoft.VSCode, but rather some other app, however the user does not bother to read the prompt and simply clicks accept.
In this way, an app that takes over the com registration of the Windows Package Manager could use the user's mistake to allow itself to escalate.
A proposed mitigation is provided in OpenCatalogAsync which is an optional call.

Another question is whether it's required to provide a wrapper api for this api to avoid callers having to use CoCreateInstance, or write their own wrapper to project into c#.
We could provide a 1 to 1 wrapper implementation in Microsoft.Management.Deployment.Client.

Naming of the items is as always a tricky question. For this api there are multiple similar apis that are relevant with regard to naming and consistency. There is the
Windows Package Manager command line which uses "source" to describe the various repositories that can host packages and "search" to describe looking up an app. 
https://docs.microsoft.com/en-us/windows/package-manager/winget/
There is the Windows::ApplicationModel::PackageCatalog which exists as a Windows API for installing packages and monitoring their installation progress.
https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.packagecatalog?view=winrt-19041
And there is Windows.Management.Deployment.PackageManager which allows packages with the packageManagement capability to install msix apps and uses "Find" to describe looking up an app
https://docs.microsoft.com/en-us/uwp/api/windows.management.deployment.packagemanager?view=winrt-19041 

I've chosen to align with the Windows APIs in using *Catalog and Find. But this may cause some confusion if callers are using both the winget command line and this API.
In particular a problem may be that since this API does not yet propose to implement adding a "source", client applications would need to work with the command line
interface in order to do that which may cause the name change from Source to AppCatalog to be particularly confusing.

# 5 API Details

```c# (but really MIDL3)
namespace Microsoft.Management.Deployment
{
    /// State of the install.
    enum InstallProgressState
    {
        /// The install is queued but not yet active. Cancellation of the IAsyncOperationWithProgress in this state will prevent the app from downloading or installing.
        Queued = 0,
        /// The installer is downloading. Cancellation of the IAsyncOperationWithProgress in this state will end the download and prevent the app from installing.
        Downloading = 1,
        /// The install is in progress. Cancellation of the IAsyncOperationWithProgress in this state will not stop the installation or the post install cleanup.
        Installing = 2,
        /// The installer has completed and cleanup actions are in progress. Cancellation of the IAsyncOperationWithProgress in this state will not stop cleanup or roll back the install.
        PostInstall = 3,
    };

    /// Progress object for the install
    /// DESIGN NOTE: percentage for the install as a whole is purposefully not included as there is no way to estimate progress when the installer is running.
    [version(1)]
    struct InstallProgress
    {
        /// State of the install
        InstallProgressState State;
        /// DESIGN NOTE: BytesDownloaded may only be available for downloads done by Windows Package Manager itself.
        /// Number of bytes downloaded if known
        UInt32 BytesDownloaded;
        /// DESIGN NOTE: BytesRequired may only be available for downloads done by Windows Package Manager itself.
        /// Number of bytes required if known
        UInt32 BytesRequired;
        /// Download percentage completed
        UInt32 DownloadPercentage;
    };

    /// Result of the install
    runtimeclass InstallResult
    {
        InstallResult();

        /// Used by a caller to correlate the install with a caller's data.
        String CorrelationId{ get; };
        /// Whether a restart is required to complete the install.
        Boolean RebootRequired { get; };
    }

    /// IMPLEMENTATION NOTE: SourceOrigin from AppInstallerRepositorySource.h
    /// Defines the origin of the app catalog details.
    enum AppCatalogOrigin
    {
        Default,
        /// User means it was added by the user and could be removed.
        User,
        /// Predefined means it came as part of the Windows Package Manager package and cannot be removed.
        Predefined,
    };

    /// IMPLEMENTATION NOTE: SourceTrustLevel from AppInstallerRepositorySource.h
    /// Defines the trust level of the app catalog.
    enum AppCatalogTrustLevel
    {
        None,
        Trusted,
    };

    /// IMPLEMENTATION NOTE: SourceDetails from AppInstallerRepositorySource.h
    /// Interface for retrieving information about an app catalog without acting on it.
    [version(1)]
    struct AppCatalogDetails
    {
        /// The name of the app catalog.
        String Name;
        /// The type of the app catalog.
        String Type;
        /// The argument used when adding the app catalog.
        String Arg;
        /// The app catalog's extra data string.
        String Data;
        /// The app catalog's unique identifier.
        String Identifier;
        /// The last time that this app catalog was updated.
        Windows.Foundation.DateTime LastUpdateTime;
        /// The origin of the app catalog.
        AppCatalogOrigin Origin;
        /// The trust level of the app catalog
        AppCatalogTrustLevel TrustLevel;
    };

    /// A metadata item of a package version.
    enum PackageVersionMetadata
    {
        /// The InstallerType of an installed package
        InstalledType,
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
    runtimeclass PackageVersion
    {
        PackageVersion();

        /// IMPLEMENTATION NOTE: PackageVersionMetadata fields from AppInstallerRepositorySearch.h
        /// DESIGN NOTE: These would be a good candidate to leave out in V1. Maybe just keep InstalledLocation and InstalledScope?
        /// Gets any metadata associated with this package version.
        /// Primarily stores data on installed packages.
        /// Metadata fields may have no value (e.g. packages that aren't installed will not have an InstalledLocation).
        String GetMetadata(PackageVersionMetadata metadataType);
        /// IMPLEMENTATION NOTE: PackageVersionProperty fields from AppInstallerRepositorySearch.h
        String Id { get; };
        String Name { get; };
        String AppCatalogIdentifier { get; };
        String AppCatalogName { get; };
        String Version { get; };
        String Channel { get; };
        /// DESIGN NOTE: RelativePath from AppInstallerRepositorySearch.h is excluded as not needed.
        /// String RelativePath;

        /// IMPLEMENTATION NOTE: PackageVersionMultiProperty fields from AppInstallerRepositorySearch.h
        /// PackageFamilyName and ProductCode can have multiple values.
        Windows.Foundation.Collections.IVectorView<String> PackageFamilyName { get; };
        Windows.Foundation.Collections.IVectorView<String> ProductCode { get; };

        /// Gets the app catalog  where this package version is from.
        AppCatalog AppCatalog { get; };

        /// DESIGN NOTE:
        /// GetManifest from IPackageVersion in AppInstallerRepositorySearch is not implemented in V1. That class has a lot of fields and no one requesting it.
        /// Gets the manifest of this package version.
        /// virtual Manifest::Manifest GetManifest() = 0;
    };

    /// IMPLEMENTATION NOTE: PackageVersionKey from AppInstallerRepositorySearch.h
    /// A key to identify a package version within a package.
    [version(1)]
    struct PackageVersionId
    {
        /// The app catalog id that this version came from.
        String AppCatalogId;
        /// The version.
        String Version;
        /// The channel.
        String Channel;
    };

    /// IMPLEMENTATION NOTE: IPackage from AppInstallerRepositorySearch.h
    /// A package, potentially containing information about it's local state and the available versions.
    runtimeclass Package
    {
        Package();

        /// IMPLEMENTATION NOTE: This would be new work as Windows Package Manager does not currently create a Package object from a manifest.
        /// DESIGN NOTE: This is proposed as the preferred alternative to having InstallOptions match the winget command line which
        /// allows you to pass in a manifestPath directly into install. Allowing creation of a Package object from a manifest allows the install api to 
        /// consistently use package objects rather than having a "you can either set the manifest path or use a package but not both" problem.
        Package(String manifestPath);

        /// IMPLEMENTATION NOTE: PackageProperty fields from AppInstallerRepositorySearch.h
        /// Gets a property of this package.
        String Id { get; };
        String Name { get; };

        /// Gets the installed package information if the package is installed.
        PackageVersion InstalledVersion{ get; };

        /// Gets all available versions of this package.
        /// The versions will be returned in sorted, descending order.
        ///  Ex. { 4, 3, 2, 1 }
        Windows.Foundation.Collections.IVectorView<PackageVersionId> AvailableVersions { get; };

        /// Gets a specific version of this package.
        PackageVersion LatestAvailableVersion { get; };

        /// Gets a specific version of this package.
        PackageVersion GetAvailableVersion(PackageVersionId versionKey);

        /// Gets a value indicating whether an available version is newer than the installed version.
        Boolean IsUpdateAvailable { get; };

        /// Gets whether the package is installing
        Boolean IsInstalling { get; };
        /// Gets the installing progress if the package is installing
        Windows.Foundation.IAsyncOperationWithProgress<InstallResult, InstallProgress>  InstallProgress{ get; };

        /// DESIGN NOTE:
        /// IsSame from IPackage in AppInstallerRepositorySearch is not implemented in V1.
        /// Determines if the given IPackage refers to the same package as this one.
        /// virtual bool IsSame(const IPackage*) const = 0;
    }

    /// IMPLEMENTATION NOTE: CompositeSearchBehavior from AppInstallerRepositorySource.h
    /// Search behavior for composite sources.
    /// Only relevant for composite sources with a local source, not for aggregates of multiple available sources.
    /// Installed and available packages in the result are always correlated when possible.
    enum CompositeSearchBehavior
    {
        /// Search only installed packages
        Installed = 0,
        /// Search only installing packages
        Installing = 1,
        /// Search only local packages (installed and installing)
        AllLocal = 2,
        /// Search both local and remote sources.
        AllPackages = 3,
    };

    /// IMPLEMENTATION NOTE: MatchType from AppInstallerRepositorySearch.h
    enum MatchType
    {
        Exact = 0,
        CaseInsensitive = 1,
        StartsWith = 2,
        Fuzzy = 3,
        Substring = 4,
        FuzzySubstring = 5,
        Wildcard = 6,
    };
    /// IMPLEMENTATION NOTE: PackageMatchField from AppInstallerRepositorySearch.h
    /// The field to match on.
    /// The values must be declared in order of preference in search results.
    enum PackageMatchField
    {
        AppCatalogDefined = 0,
        Id = 1,
        Name = 2,
        Moniker = 3,
        Command = 4,
        Tag = 5,
        /// DESIGN NOTE: The following PackageMatchField from AppInstallerRepositorySearch.h are not implemented in V1.
        /// PackageFamilyName,
        /// ProductCode,
        /// NormalizedNameAndPublisher,
    };
    /// IMPLEMENTATION NOTE: PackageMatchFilter from AppInstallerRepositorySearch.h
    [version(1)]
    struct PackageMatchFilter
    {
        /// Whether the Package must match the filter in order to be included in the matches.
        Boolean IsAdditive;
        /// The type of string comparison for matching
        MatchType Type;
        /// The field to search
        PackageMatchField Field;
        /// The value to match
        String Value;
        /// DESIGN NOTE: "Additional" from RequestMatch AppInstallerRepositorySearch.h is not implememented here.
    };

    /// IMPLEMENTATION NOTE: ResultMatch from AppInstallerRepositorySearch.h
    /// A single result from the search.
    runtimeclass ResultMatch
    {
        ResultMatch();

        /// The package found by the search request.
        Package Package { get; };

        /// The highest order field on which the package matched the search.
        PackageMatchFilter MatchCriteria { get; };
    }
    /// IMPLEMENTATION NOTE: SearchResult from AppInstallerRepositorySearch.h
    /// Search result data returned from FindPackages
    runtimeclass FindPackagesResult
    {
        FindPackagesResult();

        /// The full set of results from the search.
        Windows.Foundation.Collections.IVectorView<ResultMatch> Matches { get; };

        /// If true, the results were truncated by the given SearchRequest::MaximumResults.
        /// USAGE NOTE: Windows Package Manager does not support result pagination, there is no way to continue getting more results.
        Boolean IsTruncated{ get; };
    }

    /// Options for FindPackages
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
        /// Query is PackageMatchField::AppCatalogDefined and PackageMatchFilter.IsAdditive = true
        /// Inclusions are PackageMatchFilter.IsAdditive = true
        /// Filters are PackageMatchFilter.IsAdditive = false

        /// Filters to find packages
        /// USAGE NOTE: Only one filter with PackageMatchField::AppCatalogDefined is allowed.
        Windows.Foundation.Collections.IVector<PackageMatchFilter> Filters;

        /// Restricts the length of the returned results to the specified count.
        UInt32 Limit;

        /// Overrides the default search behavior for this search if the catalog is a composite catalog.
        /// IMPLEMENTATION NOTE: Windows Package Manager currently only supports setting the search behavior when creating the source
        /// so this is new work but is expected to be very small.
        CompositeSearchBehavior CompositeSearchBehavior;
    }

    /// IMPLEMENTATION NOTE: ISource from AppInstallerRepositorySource.h
    /// A catalog for searching for packages
    runtimeclass AppCatalog
    {
        AppCatalog();

        /// Gets a value indicating whether this app catalog is a composite of other app catalogs,
        /// and thus the packages may come from disparate app catalogs as well.
        Boolean IsComposite { get; };
        /// The details of the app catalog if it is not a composite.
        AppCatalogDetails Details { get; };

        /// Opens a catalog. Required before searching. For remote catalogs (i.e. note Installed and Installing) this may require downloading information from a server.
        Windows.Foundation.IAsyncAction OpenAsync();
        /// Searches for Packages in the catalog.
        Windows.Foundation.IAsyncOperation<FindPackagesResult> FindPackagesAsync(FindPackagesOptions options);
    }
    /// Result when using GetAppCatalogsAsync
    runtimeclass GetAppCatalogsResult
    {
        GetAppCatalogsResult();

        /// IMPLEMENTATION NOTE: This is a list of sources returned by  Windows Package Manager source list excluding the known sources.
        Windows.Foundation.Collections.IVectorView<AppCatalog> AppCatalogs { get; };
    }

    /// Catalogs with AppCatalogOrigin Predefined
    enum PredefinedAppCatalog
    {
        OpenWindowsCatalog = 0,
        InstalledPackages = 1,
        /// IMPLEMENTATION NOTE:  Windows Package Manager does not currently have this concept. 
        /// This would be a new Orchestration feature to keep track of the installing Package objects and allow search of those objects.
        /// A less costly version of this might skip the search feature and use something like AppInstaller.GetInstallingPackagesAsync to just return a list of Packages.
        /// The current expectation is that Installing packages will be stored using the same database tech that InstalledPackages are using and will be able to reuse that code.
        InstallingPackages = 2
    };

    /// 
    runtimeclass GetCompositeAppCatalogOptions
    {
        GetCompositeAppCatalogOptions(); 

        /// Get a composite catalog to allow searching a user defined or pre defined source and a local source (Installing, Installed) together  
        /// IMPLEMENTATION NOTE: Windows Package Manager currently only supports one local source and one remote source per composite catalog.
        /// DESIGN NOTE: It seems likely that in the future we will want to support letting callers use two local sources (i.e. both Installing and Installed)
        IVector<AppCatalog> Catalogs;
        /// Sets the default search behavior if the catalog is a composite catalog.
        CompositeSearchBehavior CompositeSearchBehavior;
    }

    enum InstallScope
    {
        User = 0,
        Machine = 1,
    };

    enum InstallMode
    {
        /// The default experience for the installer. Installer may show some UI.
        Default = 0,
        /// Runs the installer in silent mode. This suppresses the installer's UI to the extent possible (installer may still show some required UI).
        Silent = 1,
        /// Runs the installer in interactive mode.
        Interactive = 2,
    };

    /// Options when installing a package.
    /// Intended to allow full compatibility with the "winget install" command line interface.
    runtimeclass InstallOptions
    {
        InstallOptions();

        /// DESIGN NOTE: --Exact from the winget install command line is implied here by virtue of the find then install object semantics.
        /// Likewise --Query from the command line is not implemented here since the query is required up front in order to get the Package object.
        /// DESIGN NOTE: Using PackageVersion rather than Package felt like it would be too specific here. Package and PackageVersionId (or if unspecified then the latest one) may be nicer
        /// if we end up having versions that have different applicability or other more complex logic about which version is preferred.
        Package Package;
        /// Optionally specifies the version from the package to install. If unspecified the version matching Package.GetLatestVersion() is used.
        PackageVersionId PackageVersionId;

        /// Specifies alternate location to install package (if supported).
        String PreferredInstallLocation;
        /// User or Machine. 
        InstallScope InstallScope;
        /// Silent, Interactive, or Default
        InstallMode InstallMode;
        /// Directs the logging to a log file. Callers must provide a path to a file that the Windows Package Manager package has write access to.
        String LogOutputPath;
        /// Continues the install even if the hash in the catalog does not match the linked installer.
        Boolean AllowHashMismatch;
        /// A string that will be passed to the installer. 
        /// IMPLEMENTATION NOTE: maps to "--override" in the winget cmd line
        String ReplacementInstallerArguments;

        /// Used by a caller to correlate the install with a caller's data.
        /// IMPLEMENTATION NOTE: This area is in flux but right now it's expected that this would allow a caller to use JSON to encode multiple arguments.
        String AdditionalTelemetryArguments;
        /// A string that will be passed to the source server if using a REST source
        String AdditionalAppCatalogArguments;
    }

    runtimeclass AppInstaller
    {
        AppInstaller();

        /// Get the available catalogs. Each source will have a separate catalog. This list does not include KnownCatalogs.
        /// This does not open the catalog. These catalogs can be used individually or merged with GetCompositeAppCatalogAsync. 
        /// IMPLEMENTATION NOTE: This is a list of sources returned by Windows Package Manager source list excluding the predefined sources.
        Windows.Foundation.IAsyncOperation< Windows.Foundation.Collections.IVectorView<AppCatalog> > GetAppCatalogsAsync();
        /// Get a built in catalog
        [default_overload]
        Windows.Foundation.IAsyncOperation<AppCatalog> GetAppCatalogAsync(PredefinedAppCatalog predefinedAppCatalog);
        /// Get a catalog by a known unique identifier
        Windows.Foundation.IAsyncOperation<AppCatalog> GetAppCatalogAsync(String catalogId);
        /// Get a composite catalog to allow searching a user defined or pre defined source and a local source (Installing, Installed) together  
        Windows.Foundation.IAsyncOperation<AppCatalog> GetCompositeAppCatalogAsync(GetCompositeAppCatalogOptions options);

        /// Install the specified package
        Windows.Foundation.IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackageAsync(InstallOptions options);
    }
}


```

# Appendix

