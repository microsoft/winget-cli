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
        [Description("Find installed and available packages using WinGet")]
        public CallToolResponse FindPackages(
            [Description("Find packages identified by this value")] string query)
        {
            try
            {
                ToolResponse.CheckGroupPolicy();

                var catalog = ConnectCatalog();

                // First attempt a more exact match
                var findResult = FindForQuery(catalog, query, fullStringMatch: true);

                // If nothing is found, expand to a looser search
                if ((findResult.Matches?.Count ?? 0) == 0)
                {
                    findResult = FindForQuery(catalog, query, fullStringMatch: false);
                }

                if (findResult.Status != FindPackagesResultStatus.Ok)
                {
                    return PackageResponse.ForFindError(findResult);
                }

                List<FindPackageResult> contents = new List<FindPackageResult>();
                contents.AddPackages(findResult);

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
        [Description("Install or update a package using WinGet")]
        public async Task<CallToolResponse> InstallPackage(
            [Description("The identifier of the WinGet package")] string identifier,
            IProgress<ProgressNotificationValue> progress,
            CancellationToken cancellationToken,
            [Description("The source containing the package")] string? source = null)
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
                    findResult = ReFindForPackage(catalogPackage.DefaultInstallVersion);
                }

                return PackageResponse.ForInstallOperation(installResult, findResult);
            }
            catch (ToolResponseException e)
            {
                return e.Response;
            }
        }

        private ConnectResult ConnectCatalogWithResult(string? catalog = null)
        {
            CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = new CreateCompositePackageCatalogOptions();

            var catalogs = packageManager.GetPackageCatalogs();
            for (int i = 0; i < catalogs.Count; ++i)
            {
                var catalogRef = catalogs[i];
                if (string.IsNullOrEmpty(catalog) || catalogRef?.Info.Id == catalog)
                {
                    createCompositePackageCatalogOptions.Catalogs.Add(catalogs[i]);
                }
            }
            createCompositePackageCatalogOptions.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;

            var compositeRef = packageManager.CreateCompositePackageCatalog(createCompositePackageCatalogOptions);
            return compositeRef.Connect();
        }

        private PackageCatalog ConnectCatalog(string? catalog = null)
        {
            var result = ConnectCatalogWithResult(catalog);
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
            string? message = null;

            switch (installProgress.State)
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
                Progress = (float)((installProgress.DownloadProgress * downloadPercentage) + (installProgress.InstallationProgress * (1.0f - downloadPercentage))),
                Message = message,
            };

            return result;
        }
    }
}
