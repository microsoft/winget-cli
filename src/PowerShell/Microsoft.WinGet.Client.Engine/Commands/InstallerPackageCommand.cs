// -----------------------------------------------------------------------------
// <copyright file="InstallerPackageCommand.cs" company="Microsoft Corporation">
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
    /// Installs or updates a package from the pipeline or from a configured source.
    /// </summary>
    public sealed class InstallerPackageCommand : InstallCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallerPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="override">Override arguments to be passed on to the installer.</param>
        /// <param name="custom">Additional arguments.</param>
        /// <param name="location">Installation location.</param>
        /// <param name="allowHashMismatch">To skip the installer hash validation check.</param>
        /// <param name="force">To continue upon non security related failures.</param>
        /// <param name="header">HTTP Header to pass on to the REST Source.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage.</param>
        /// <param name="version">Version to install.</param>
        /// <param name="log">Logging file location.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        /// <param name="skipDependencies">To skip package dependencies.</param>
        public InstallerPackageCommand(
            PSCmdlet psCmdlet,
            string @override,
            string custom,
            string location,
            bool allowHashMismatch,
            bool force,
            string header,
            PSCatalogPackage psCatalogPackage,
            string version,
            string log,
            string id,
            string name,
            string moniker,
            string source,
            string[] query,
            bool skipDependencies)
            : base(psCmdlet)
        {
            // InstallCommand.
            this.Override = @override;
            this.Custom = custom;
            this.Location = location;
            this.Force = force;
            this.Header = header;
            this.AllowHashMismatch = allowHashMismatch;
            this.SkipDependencies = skipDependencies;
            this.Log = log;

            // PackageCommand.
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
        }

        /// <summary>
        /// Process install package command.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageInstallScope">PSPackageInstallScope.</param>
        /// <param name="psProcessorArchitecture">PSProcessorArchitecture.</param>
        /// <param name="psPackageInstallMode">PSPackageInstallMode.</param>
        /// <param name="psPackageInstallerType">PSPackageInstallerType.</param>
        public void Install(
            string psPackageFieldMatchOption,
            string psPackageInstallScope,
            string psProcessorArchitecture,
            string psPackageInstallMode,
            string psPackageInstallerType)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        InstallOptions options = this.GetInstallOptions(version, psPackageInstallMode);
                        if (!PSEnumHelpers.IsDefaultEnum(psProcessorArchitecture))
                        {
                            options.AllowedArchitectures.Clear();
                            options.AllowedArchitectures.Add(PSEnumHelpers.ToProcessorArchitecture(psProcessorArchitecture));
                        }

                        if (!PSEnumHelpers.IsDefaultEnum(psPackageInstallerType))
                        {
                            options.InstallerType = PSEnumHelpers.ToPackageInstallerType(psPackageInstallerType);
                        }

                        options.PackageInstallScope = PSEnumHelpers.ToPackageInstallScope(psPackageInstallScope);

                        return await this.InstallPackageAsync(package, options);
                    }));

            if (result != null)
            {
                this.Write(StreamType.Object, new PSInstallResult(result.Item1, result.Item2));
            }
        }

        /// <summary>
        /// Process update package command.
        /// </summary>
        /// <param name="includeUnknown">If updating to an unknown version is allowed.</param>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageInstallScope">PSPackageInstallScope.</param>
        /// <param name="psProcessorArchitecture">PSProcessorArchitecture.</param>
        /// <param name="psPackageInstallMode">PSPackageInstallMode.</param>
        /// <param name="psPackageInstallerType">PSPackageInstallerType.</param>
        public void Update(
            bool includeUnknown,
            string psPackageFieldMatchOption,
            string psPackageInstallScope,
            string psProcessorArchitecture,
            string psPackageInstallMode,
            string psPackageInstallerType)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.LocalCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        InstallOptions options = this.GetInstallOptions(version, psPackageInstallMode);
                        options.AllowUpgradeToUnknownVersion = includeUnknown;

                        if (!PSEnumHelpers.IsDefaultEnum(psProcessorArchitecture))
                        {
                            options.AllowedArchitectures.Clear();
                            options.AllowedArchitectures.Add(PSEnumHelpers.ToProcessorArchitecture(psProcessorArchitecture));
                        }

                        if (!PSEnumHelpers.IsDefaultEnum(psPackageInstallerType))
                        {
                            options.InstallerType = PSEnumHelpers.ToPackageInstallerType(psPackageInstallerType);
                        }

                        options.PackageInstallScope = PSEnumHelpers.ToPackageInstallScope(psPackageInstallScope);

                        return await this.UpgradePackageAsync(package, options);
                    }));

            if (result != null)
            {
                this.Write(StreamType.Object, new PSInstallResult(result.Item1, result.Item2));
            }
        }

        private async Task<InstallResult> InstallPackageAsync(
            CatalogPackage package,
            InstallOptions options)
        {
            var installOperation = new InstallOperationWithProgress(
                this,
                string.Format(Resources.ProgressRecordActivityInstalling, package.Name));
            return await installOperation.ExecuteAsync(
                    () => PackageManagerWrapper.Instance.InstallPackageAsync(package, options));
        }

        private async Task<InstallResult> UpgradePackageAsync(
            CatalogPackage package,
            InstallOptions options)
        {
            var installOperation = new InstallOperationWithProgress(
                this,
                string.Format(Resources.ProgressRecordActivityUpdating, package.Name));
            return await installOperation.ExecuteAsync(
                    () => PackageManagerWrapper.Instance.UpgradePackageAsync(package, options));
        }
    }
}
