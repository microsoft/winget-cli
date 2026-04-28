// -----------------------------------------------------------------------------
// <copyright file="WingetPackageTools.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer
{
    using System.ComponentModel;
    using Microsoft.Management.Deployment;
    using ModelContextProtocol.Protocol;
    using ModelContextProtocol.Server;
    using ModelContextProtocol;
    using Windows.Foundation;
    using WinGetMCPServer.Extensions;
    using WinGetMCPServer.Response;
    using WinGetMCPServer.Exceptions;

    /// <summary>
    /// WinGet package tools.
    /// </summary>
    [McpServerToolType]
    internal class WingetPackageTools
    {
        private PackageManager packageManager;

        public WingetPackageTools()
        {
            packageManager = ServerConnection.Instance;
        }

        [McpServerTool(
            Name = "find-winget-packages",
            Title = "Find WinGet Packages",
            ReadOnly = true,
            OpenWorld = false)]
        [Description("Find installed and available packages using WinGet. To list all installed packages that have available upgrades (equivalent to 'winget upgrade'), call with upgradeable=true and no query. To filter upgradeable packages by name, call with upgradeable=true and a query. To search for packages to install, call with upgradeable=false and a required query.")]
        public CallToolResult FindPackages(
            [Description("Find packages identified by this value. Required when upgradeable is false; optionally filters results when upgradeable is true.")] string? query = null,
            [Description("When true, only return installed packages that have available upgrades")] bool upgradeable = false)
        {
            try
            {
                ToolResponse.CheckGroupPolicy();

                if (!upgradeable && string.IsNullOrEmpty(query))
                {
                    return new CallToolResult()
                    {
                        IsError = true,
                        Content = [new TextContentBlock() { Text = "A query is required when upgradeable is false" }],
                    };
                }

                // Use LocalCatalogs when listing upgrades to enumerate only installed packages,
                // consistent with `winget upgrade`. Remote catalogs are still included in the
                // composite so IsUpdateAvailable remains accurate.
                var catalog = ConnectCatalog(searchBehavior: upgradeable
                    ? CompositeSearchBehavior.LocalCatalogs
                    : CompositeSearchBehavior.AllCatalogs);

                FindPackagesResult findResult;
                if (string.IsNullOrEmpty(query))
                {
                    // This can only happen in the case that upgradeable is true, in which case this
                    // won't accidentally list all packages from all catalogs
                    findResult = FindAllPackages(catalog);
                }
                else
                {
                    // First attempt a more exact match
                    findResult = FindForQuery(catalog, query, fullStringMatch: true);

                    // If nothing is found, expand to a looser search
                    if ((findResult.Matches?.Count ?? 0) == 0)
                    {
                        findResult = FindForQuery(catalog, query, fullStringMatch: false);
                    }
                }

                if (findResult.Status != FindPackagesResultStatus.Ok)
                {
                    return PackageResponse.ForFindError(findResult);
                }

                List<FindPackageResult> contents = new List<FindPackageResult>();
                if (upgradeable)
                {
                    for (int i = 0; i < findResult.Matches?.Count; ++i)
                    {
                        var package = findResult.Matches[i].CatalogPackage;
                        if (package.IsUpdateAvailable)
                        {
                            contents.Add(PackageListExtensions.FindPackageResultFromCatalogPackage(package));
                        }
                    }
                }
                else
                {
                    contents.AddPackages(findResult);
                }

                return ToolResponse.FromObject(contents);
            }
            catch (ToolResponseException e)
            {
                return e.Response;
            }
        }

        [McpServerTool(
            Name = "install-winget-package",
            Title = "Install WinGet Package",
            ReadOnly = false,
            Destructive = true,
            Idempotent = false,
            OpenWorld = false)]
        [Description("Install or upgrade a package using WinGet. When upgradeOnly is true, only upgrades an already-installed package and returns an error if it is not installed. When upgradeOnly is false (default), installs the package if not present or upgrades it if already installed.")]
        public async Task<CallToolResult> InstallPackage(
            [Description("The identifier of the WinGet package")] string identifier,
            IProgress<ProgressNotificationValue> progress,
            CancellationToken cancellationToken,
            [Description("The source containing the package")] string? source = null,
            [Description("When true, only upgrade an already-installed package; returns an error if the package is not installed")] bool upgradeOnly = false)
        {
            try
            {
                ToolResponse.CheckGroupPolicy();

                var packageCatalog = ConnectCatalog(source);

                if (cancellationToken.IsCancellationRequested)
                {
                    return PackageResponse.ForCancelBeforeSystemChange();
                }

                // First attempt a more exact match
                var findResult = FindForIdentifier(packageCatalog, identifier, expandedFields: false);

                if (cancellationToken.IsCancellationRequested)
                {
                    return PackageResponse.ForCancelBeforeSystemChange();
                }

                // If nothing is found, expand to a looser search
                if ((findResult.Matches?.Count ?? 0) == 0)
                {
                    findResult = FindForIdentifier(packageCatalog, identifier, expandedFields: true);
                }

                if (findResult.Status != FindPackagesResultStatus.Ok)
                {
                    return PackageResponse.ForFindError(findResult);
                }

                if (findResult.Matches?.Count == 0)
                {
                    return PackageResponse.ForEmptyFind(identifier, source);
                }
                else if (findResult.Matches?.Count > 1)
                {
                    return PackageResponse.ForMultiFind(identifier, source, findResult);
                }

                CatalogPackage catalogPackage = findResult.Matches![0].CatalogPackage;

                if (upgradeOnly && catalogPackage.InstalledVersion == null)
                {
                    return PackageResponse.ForNotInstalled(identifier, source);
                }

                InstallOptions options = new InstallOptions();
                IAsyncOperationWithProgress<InstallResult, InstallProgress>? operation = null;

                if (cancellationToken.IsCancellationRequested)
                {
                    return PackageResponse.ForCancelBeforeSystemChange();
                }

                if (catalogPackage.InstalledVersion == null)
                {
                    operation = packageManager.InstallPackageAsync(catalogPackage, options);
                }
                else
                {
                    operation = packageManager.UpgradePackageAsync(catalogPackage, options);
                }

                operation.Progress = (asyncInfo, progressInfo) => progress.Report(CreateInstallProgressNotification(ref progressInfo));
                using CancellationTokenRegistration registration = cancellationToken.Register(() => operation.Cancel());

                var installResult = await operation;
                findResult = null;

                if (installResult.Status == InstallResultStatus.Ok)
                {
                    // Send a completed progress entry in the event that async progress forwarding didn't
                    progress.Report(CreateInstallProgressNotification(PackageInstallProgressState.Finished, 1.0, 1.0));
                    findResult = ReFindForPackage(catalogPackage.DefaultInstallVersion);
                }

                return upgradeOnly
                    ? PackageResponse.ForUpgradeOperation(installResult, findResult)
                    : PackageResponse.ForInstallOperation(installResult, findResult);
            }
            catch (ToolResponseException e)
            {
                return e.Response;
            }
        }

        private ConnectResult ConnectCatalogWithResult(string? catalog = null, CompositeSearchBehavior searchBehavior = CompositeSearchBehavior.AllCatalogs)
        {
            CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = new CreateCompositePackageCatalogOptions();

            var catalogs = packageManager.GetPackageCatalogs();
            for (int i = 0; i < catalogs.Count; ++i)
            {
                var catalogRef = catalogs[i];
                if ((string.IsNullOrEmpty(catalog) && !catalogRef.Info.Explicit)
                    || catalogRef?.Info.Name == catalog)
                {
                    createCompositePackageCatalogOptions.Catalogs.Add(catalogRef);
                }
            }
            createCompositePackageCatalogOptions.CompositeSearchBehavior = searchBehavior;

            var compositeRef = packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions);
            return compositeRef.Connect();
        }

        private PackageCatalog ConnectCatalog(string? catalog = null, CompositeSearchBehavior searchBehavior = CompositeSearchBehavior.AllCatalogs)
        {
            var result = ConnectCatalogWithResult(catalog, searchBehavior);
            if (result.Status != ConnectResultStatus.Ok)
            {
                throw new ToolResponseException(PackageResponse.ForConnectError(result));
            }
            return result.PackageCatalog;
        }

        private FindPackagesResult FindForQuery(PackageCatalog catalog, string query, bool fullStringMatch)
        {
            PackageFieldMatchOption fullStringMatchOption = fullStringMatch ? PackageFieldMatchOption.EqualsCaseInsensitive : PackageFieldMatchOption.ContainsCaseInsensitive;

            FindPackagesOptions findPackageOptions = new();
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Id, Option = fullStringMatchOption, Value = query });
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Name, Option = fullStringMatchOption, Value = query });
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Moniker, Option = PackageFieldMatchOption.EqualsCaseInsensitive, Value = query });

            return catalog!.FindPackages(findPackageOptions);
        }

        private FindPackagesResult FindForIdentifier(PackageCatalog catalog, string query, bool expandedFields)
        {
            FindPackagesOptions findPackageOptions = new();
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Id, Option = PackageFieldMatchOption.EqualsCaseInsensitive, Value = query });

            if (expandedFields)
            {
                findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Name, Option = PackageFieldMatchOption.EqualsCaseInsensitive, Value = query });
                findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Moniker, Option = PackageFieldMatchOption.EqualsCaseInsensitive, Value = query });
            }

            return catalog!.FindPackages(findPackageOptions);
        }

        private FindPackagesResult FindAllPackages(PackageCatalog catalog)
        {
            FindPackagesOptions findPackageOptions = new();
            return catalog!.FindPackages(findPackageOptions);
        }

        private FindPackagesResult? ReFindForPackage(PackageVersionInfo packageVersionInfo)
        {
            var connectResult = ConnectCatalogWithResult(packageVersionInfo.PackageCatalog.Info.Id);

            if (connectResult.Status != ConnectResultStatus.Ok)
            {
                return null;
            }

            var catalog = connectResult.PackageCatalog;

            FindPackagesOptions findPackageOptions = new();
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Id, Option = PackageFieldMatchOption.Equals, Value = packageVersionInfo.Id });

            return catalog!.FindPackages(findPackageOptions);
        }

        private static ProgressNotificationValue CreateInstallProgressNotification(ref InstallProgress installProgress)
        {
            return CreateInstallProgressNotification(installProgress.State, installProgress.DownloadProgress, installProgress.InstallationProgress);
        }

        private static ProgressNotificationValue CreateInstallProgressNotification(PackageInstallProgressState state, double downloadProgress, double installProgress)
        {
            string? message = null;

            switch (state)
            {
                case PackageInstallProgressState.Queued:
                    message = "The install operation is queued";
                    break;
                case PackageInstallProgressState.Downloading:
                    message = "The package installer is being downloaded";
                    break;
                case PackageInstallProgressState.Installing:
                    message = "The package is being installed";
                    break;
                case PackageInstallProgressState.PostInstall:
                    message = "The installation operation is wrapping up";
                    break;
                case PackageInstallProgressState.Finished:
                    message = "The install is complete";
                    break;
                default:
                    message = "Unknown install state";
                    break;
            }

            const float downloadPercentage = 0.8f;

            ProgressNotificationValue result = new ProgressNotificationValue()
            {
                Progress = (float)((downloadProgress * downloadPercentage) + (installProgress * (1.0f - downloadPercentage))),
                Message = message,
            };

            return result;
        }
    }
}
