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
    public sealed class DownloadCommand : PackageInstallerCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DownloadCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage.</param>
        /// <param name="version">Version to install.</param>
        /// <param name="log">Logging file location.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        /// <param name="allowHashMismatch">To skip the installer hash validation check.</param>
        /// <param name="skipDependencies">To skip package dependencies.</param>
        /// <param name="locale">Package locale.</param>
        /// <param name="scope">Package installer scope.</param>
        /// <param name="architecture">Package installer architecture.</param>
        /// <param name="installerType">Package installer type.</param>
        /// <param name="matchOption">Package field match option.</param>
        public DownloadCommand(
            PSCmdlet psCmdlet,
            PSCatalogPackage psCatalogPackage,
            string version,
            string log,
            string id,
            string name,
            string moniker,
            string source,
            string[] query,
            bool allowHashMismatch,
            bool skipDependencies,
            string locale,
            PSPackageInstallScope scope,
            PSProcessorArchitecture architecture,
            PSPackageInstallerType installerType,
            PSPackageFieldMatchOption matchOption)
            : base(psCmdlet)
        {
            // PackageCommand.
            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage;
            }

            this.Version = version;
            this.Log = log;

            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;
            this.MatchOption = matchOption;

            // PackageInstallerCommand.
            this.AllowHashMismatch = allowHashMismatch;
            this.SkipDependencies = skipDependencies;
            this.InstallerType = installerType;
            this.Locale = locale;
            this.Scope = scope;
            this.Architecture = architecture;
        }

        /// <summary>
        /// Process download package.
        /// </summary>
        /// <param name="downloadDirectory">The target directory where the installer will be downloaded to.</param>
        public void Download(string downloadDirectory)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.LocalCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(this.MatchOption),
                    async (package, version) =>
                    {
                        DownloadOptions options = this.GetDownloadOptions(version);

                        if (!string.IsNullOrEmpty(downloadDirectory))
                        {
                            options.DownloadDirectory = downloadDirectory;
                        }

                        if (!string.IsNullOrEmpty(this.Locale))
                        {
                            options.Locale = this.Locale;
                        }

                        options.AllowHashMismatch = this.AllowHashMismatch;
                        options.SkipDependencies = this.SkipDependencies;

                        options.Architecture = PSEnumHelpers.ToProcessorArchitecture(this.Architecture);
                        options.InstallerType = PSEnumHelpers.ToPackageInstallerType(this.InstallerType);
                        options.Scope = PSEnumHelpers.ToPackageInstallScope(this.Scope);

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

            // TODO: Add support for specifying log output.
            if (version != null)
            {
                options.PackageVersionId = version;
            }

            return options;
        }

        private async Task<DownloadResult> DownloadPackageAsync(
            CatalogPackage package,
            DownloadOptions options)
        {
            var progressOperation = new DownloadOperationWithProgress(
                this,
                string.Format(Resources.ProgressRecordActivityDownloading, package.Name));
            return await progressOperation.ExecuteAsync(
                    () => PackageManagerWrapper.Instance.DownloadPackageAsync(package, options));
        }
    }
}
