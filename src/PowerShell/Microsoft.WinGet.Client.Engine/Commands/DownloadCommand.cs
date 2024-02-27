// -----------------------------------------------------------------------------
// <copyright file="DownloadCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Downloads a package installer.
    /// </summary>
    public sealed class DownloadCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DownloadCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage.</param>
        /// <param name="version">Version to install.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        /// <param name="allowHashMismatch">To skip the installer hash validation check.</param>
        /// <param name="skipDependencies">To skip package dependencies.</param>
        /// <param name="locale">Locale of the package.</param>
        public DownloadCommand(
            PSCmdlet psCmdlet,
            PSCatalogPackage psCatalogPackage,
            string version,
            string id,
            string name,
            string moniker,
            string source,
            string[] query,
            bool allowHashMismatch,
            bool skipDependencies,
            string locale)
            : base(psCmdlet)
        {
            // PackageCommand
            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage;
            }

            this.Version = version;

            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;

            // DownloadCommand
            this.AllowHashMismatch = allowHashMismatch;
            this.SkipDependencies = skipDependencies;
            this.Locale = locale;
        }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        private bool AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip dependencies.
        /// </summary>
        private bool SkipDependencies { get; set; }

        /// <summary>
        /// Gets or sets the locale to install.
        /// </summary>
        private string? Locale { get; set; }

        /// <summary>
        /// Process download package.
        /// </summary>
        /// <param name="downloadDirectory">The target directory where the installer will be downloaded to.</param>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageInstallScope">PSPackageInstallScope.</param>
        /// <param name="psProcessorArchitecture">PSProcessorArchitecture.</param>
        /// <param name="psPackageInstallerType">PSPackageInstallerType.</param>
        public void Download(
            string downloadDirectory,
            string psPackageFieldMatchOption,
            string psPackageInstallScope,
            string psProcessorArchitecture,
            string psPackageInstallerType)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        DownloadOptions options = this.GetDownloadOptions(version);

                        if (!string.IsNullOrEmpty(downloadDirectory))
                        {
                            options.DownloadDirectory = downloadDirectory;
                        }

                        if (!PSEnumHelpers.IsDefaultEnum(psProcessorArchitecture))
                        {
                            options.Architecture = PSEnumHelpers.ToProcessorArchitecture(psProcessorArchitecture);
                        }

                        if (!PSEnumHelpers.IsDefaultEnum(psPackageInstallerType))
                        {
                            options.InstallerType = PSEnumHelpers.ToPackageInstallerType(psPackageInstallerType);
                        }

                        options.Scope = PSEnumHelpers.ToPackageInstallScope(psPackageInstallScope);

                        return await this.DownloadPackageAsync(package, options);
                    }));

            if (result != null)
            {
                this.Write(StreamType.Object, new PSDownloadResult(result.Item1, result.Item2));
            }
        }

        private DownloadOptions GetDownloadOptions(PackageVersionId? version)
        {
            var options = ManagementDeploymentFactory.Instance.CreateDownloadOptions();
            if (version != null)
            {
                options.PackageVersionId = version;
            }

            if (this.Locale != null)
            {
                options.Locale = this.Locale;
            }

            options.AllowHashMismatch = this.AllowHashMismatch;
            options.SkipDependencies = this.SkipDependencies;

            return options;
        }

        private async Task<DownloadResult> DownloadPackageAsync(
            CatalogPackage package,
            DownloadOptions options)
        {
            var activity = string.Format(Resources.ProgressRecordActivityExporting, package.Name);
            var progressOperation = new DownloadOperationWithProgress(this, activity);
            return await progressOperation.ExecuteAsync(
                () => PackageManagerWrapper.Instance.DownloadPackageAsync(package, options));
        }
    }
}
