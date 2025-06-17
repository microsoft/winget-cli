using Microsoft.Management.Deployment;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;
using System.ComponentModel;
using System.Text.Json.Serialization;
using System.Text.Json;
using ModelContextProtocol;
using Windows.Foundation;

namespace WinGetMCPServer
{
    /// <summary>
    /// Trial of winget MCP tools.
    /// </summary>
    [McpServerToolType]
    internal class WingetPackage_Reflection
    {
        private PackageManager packageManager;

        public WingetPackage_Reflection()
        {
            packageManager = new PackageManager();
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
            var connectResult = ConnectCatalog();

            if (connectResult.Status != ConnectResultStatus.Ok)
            {
                return ResponseForConnectError(connectResult);
            }

            var catalog = connectResult.PackageCatalog;

            // First attempt a more exact match
            var findResult = FindForQuery(catalog, query, fullStringMatch: true);

            // If nothing is found, expand to a looser search
            if ((findResult.Matches?.Count ?? 0) == 0)
            {
                findResult = FindForQuery(catalog, query, fullStringMatch: false);
            }

            if (findResult.Status != FindPackagesResultStatus.Ok)
            {
                return ResponseForFindError(findResult);
            }

            List<FindPackageResult> contents = new List<FindPackageResult>();
            AddFoundPackagesToList(contents, findResult);

            return ResponseFromObject(contents);
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
            [Description("The catalog containing the package")] string? catalog = null)
        {
            var connectResult = ConnectCatalog(catalog);

            if (connectResult.Status != ConnectResultStatus.Ok)
            {
                return ResponseForConnectError(connectResult);
            }

            if (cancellationToken.IsCancellationRequested)
            {
                return ResponseForCancelBeforeSystemChange();
            }

            var packageCatalog = connectResult.PackageCatalog;

            // First attempt a more exact match
            var findResult = FindForIdentifier(packageCatalog, identifier, expandedFields: false);

            // If nothing is found, expand to a looser search
            if ((findResult.Matches?.Count ?? 0) == 0)
            {
                findResult = FindForIdentifier(packageCatalog, identifier, expandedFields: true);
            }

            if (findResult.Status != FindPackagesResultStatus.Ok)
            {
                return ResponseForFindError(findResult);
            }

            if (findResult.Matches?.Count == 0)
            {
                return ResponseForEmptyFind(identifier, catalog);
            }
            else if (findResult.Matches?.Count > 1)
            {
                return ResponseForMultiFind(identifier, catalog, findResult);
            }

            CatalogPackage catalogPackage = findResult.Matches![0].CatalogPackage;
            InstallOptions options = new InstallOptions();
            IAsyncOperationWithProgress<InstallResult, InstallProgress>? operation = null;

            if (cancellationToken.IsCancellationRequested)
            {
                return ResponseForCancelBeforeSystemChange();
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

            return ResponseForInstallOperation(installResult, findResult);
        }

        private ConnectResult ConnectCatalog(string? catalog = null)
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
            var connectResult = ConnectCatalog(packageVersionInfo.PackageCatalog.Info.Id);

            if (connectResult.Status != ConnectResultStatus.Ok)
            {
                return null;
            }

            var catalog = connectResult.PackageCatalog;

            FindPackagesOptions findPackageOptions = new();
            findPackageOptions.Selectors.Add(new PackageMatchFilter() { Field = PackageMatchField.Id, Option = PackageFieldMatchOption.Equals, Value = packageVersionInfo.Id });

            return catalog!.FindPackages(findPackageOptions);
        }

        private FindPackageResult FindPackageResultFromCatalogPackage(CatalogPackage package)
        {
            FindPackageResult findPackageResult = new FindPackageResult()
            {
                Identifier = package.Id,
                Name = package.Name,
            };

            var installedVersion = package.InstalledVersion;
            findPackageResult.IsInstalled = installedVersion != null;

            if (installedVersion != null)
            {
                findPackageResult.Catalog = installedVersion.PackageCatalog?.Info?.Name;
                if (string.IsNullOrEmpty(findPackageResult.Catalog))
                {
                    findPackageResult.Catalog = package.DefaultInstallVersion?.PackageCatalog?.Info?.Name;
                }

                findPackageResult.InstalledVersion = installedVersion.Version;
                findPackageResult.IsUpdateAvailable = package.IsUpdateAvailable;

                string installLocation = installedVersion.GetMetadata(PackageVersionMetadataField.InstalledLocation);
                if (!string.IsNullOrEmpty(installLocation))
                {
                    findPackageResult.InstalledLocation = installLocation;
                }
            }
            else
            {
                findPackageResult.Catalog = package.DefaultInstallVersion.PackageCatalog.Info.Name;
            }

            return findPackageResult;
        }

        private void AddFoundPackagesToList(List<FindPackageResult> list, FindPackagesResult findResult)
        {
            for (int i = 0; i < findResult.Matches!.Count; ++i)
            {
                list.Add(FindPackageResultFromCatalogPackage(findResult.Matches[i].CatalogPackage));
            }
        }

        private CallToolResponse ResponseFromObject(object value, bool isError = false)
        {
            return new CallToolResponse()
            {
                IsError = isError,
                Content = [new Content() { Text = JsonSerializer.Serialize(value, GetDefaultJsonOptions()) }]
            };
        }

        private CallToolResponse ResponseForConnectError(ConnectResult connectResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when connecting to the WinGet package catalog with error: {connectResult.ExtendedErrorCode.Message} [0x{connectResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        private CallToolResponse ResponseForFindError(FindPackagesResult findResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when finding packages with reason {findResult.Status} and error: {findResult.ExtendedErrorCode.Message} [0x{findResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        private CallToolResponse ResponseForCancelBeforeSystemChange()
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"The operation was cancelled before any system change was started" }],
            };
        }

        private CallToolResponse ResponseForEmptyFind(string identifer, string? catalog)
        {
            PackageIdentityErrorResult result = new ()
            {
                Message = "Did not find a package with the requested identifier",
                Identifier = identifer,
                Catalog = catalog,
            };

            return ResponseFromObject(result);
        }

        private CallToolResponse ResponseForMultiFind(string identifer, string? catalog, FindPackagesResult findResult)
        {
            PackageIdentityErrorResult result = new()
            {
                Message = "Found multiple packages matching the requested identifier; provide a more specific identifier and/or catalog",
                Identifier = identifer,
                Catalog = catalog,
            };

            AddFoundPackagesToList(result.Packages, findResult);

            return ResponseFromObject(result);
        }

        private CallToolResponse ResponseForInstallOperation(InstallResult installResult, FindPackagesResult? findResult)
        {
            InstallOperationResult result = new InstallOperationResult();

            switch (installResult.Status)
            {
                case InstallResultStatus.Ok:
                    result.Message = "Install completed successfully";
                    break;
                case InstallResultStatus.BlockedByPolicy:
                    result.Message = "Installation was blocked by policy";
                    break;
                case InstallResultStatus.CatalogError:
                    result.Message = "An error occurred with the catalog";
                    break;
                case InstallResultStatus.InternalError:
                    result.Message = "An internal WinGet error occurred";
                    break;
                case InstallResultStatus.InvalidOptions:
                    result.Message = "The install options were invalid";
                    break;
                case InstallResultStatus.DownloadError:
                    result.Message = "An error occurred while downloading the package installer";
                    break;
                case InstallResultStatus.InstallError:
                    result.Message = "The package installer failed during installation";
                    break;
                case InstallResultStatus.ManifestError:
                    result.Message = "The package manifest was invalid";
                    break;
                case InstallResultStatus.NoApplicableInstallers:
                    result.Message = "No applicable package installers were available for this system";
                    break;
                case InstallResultStatus.NoApplicableUpgrade:
                    result.Message = "No applicable upgrade was available for this system";
                    break;
                case InstallResultStatus.PackageAgreementsNotAccepted:
                    result.Message = "The package requires accepting agreements; please install manually";
                    break;
                default:
                    result.Message = "Unknown install status";
                    break;
            }

            if (installResult.RebootRequired)
            {
                result.RebootRequired = true;
            }

            result.ErrorCode = installResult.ExtendedErrorCode?.HResult;

            if (installResult.Status == InstallResultStatus.InstallError)
            {
                result.InstallerErrorCode = installResult.InstallerErrorCode;
            }

            if (findResult != null && findResult.Status == FindPackagesResultStatus.Ok && findResult.Matches?.Count == 1)
            {
                result.InstalledPackageInformation = FindPackageResultFromCatalogPackage(findResult.Matches[0].CatalogPackage);
            }

            return ResponseFromObject(result, installResult.Status != InstallResultStatus.Ok);
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

        private static JsonSerializerOptions GetDefaultJsonOptions()
        {
            return new JsonSerializerOptions()
            {
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                Converters =
                {
                    new JsonStringEnumConverter(),
                },
            };
        }

        private class FindPackageResult
        {
            public required string Identifier { get; init; }

            public string? Name { get; set; }

            public string? Catalog { get; set; }

            public bool? IsInstalled { get; set; }

            public string? InstalledVersion { get; set; }

            public string? InstalledLocation { get; set; }

            public bool? IsUpdateAvailable { get; set; }
        }

        private class PackageIdentityErrorResult
        {
            public required string Message { get; init; }

            public required string Identifier { get; init; }

            public string? Catalog { get; set; }

            public List<FindPackageResult> Packages { get; set; } = new List<FindPackageResult>();
        }

        private class InstallOperationResult
        {
            public string? Message { get; set; }

            public bool? RebootRequired { get; set; }

            public int? ErrorCode { get; set; }

            public uint? InstallerErrorCode { get; set; }

            public FindPackageResult? InstalledPackageInformation { get; set; }
        }
    }
}
