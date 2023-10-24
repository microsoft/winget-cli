// -----------------------------------------------------------------------------
// <copyright file="InstallerPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.Properties;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Installs or updates a package from the pipeline or from a configured source.
    /// </summary>
    public sealed class InstallerPackageCommand : InstallCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallerPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psInstallMode">Install mode to use.</param>
        /// <param name="override">Override arguments to be passed on to the installer.</param>
        /// <param name="custom">Additional arguments.</param>
        /// <param name="location">Installation location.</param>
        /// <param name="allowHashMisMatch">To skip the installer hash validation check.</param>
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
        public InstallerPackageCommand(
            PSCmdlet psCmdlet,
            string psInstallMode,
            string @override,
            string custom,
            string location,
            bool allowHashMisMatch,
            bool force,
            string header,
            PSCatalogPackage psCatalogPackage,
            string version,
            string log,
            string id,
            string name,
            string moniker,
            string source,
            string[] query)
            : base(psCmdlet)
        {
            // InstallCommand.
            this.Override = @override;
            this.Custom = custom;
            this.Location = location;
            this.AllowHashMismatch = allowHashMisMatch;
            this.Force = force;
            this.Header = header;

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
        }

        /// <summary>
        /// Process install package command.
        /// </summary>
        /// <param name="psPackageInstallScope">PSPackageInstallScope.</param>
        /// <param name="psProcessorArchitecture">PSProcessorArchitecture.</param>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageInstallMode">PSPackageInstallMode.</param>
        public void Install(
            string psPackageInstallScope,
            string psProcessorArchitecture,
            string psPackageFieldMatchOption,
            string psPackageInstallMode)
        {
            this.GetPackageAndExecute(
                CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs,
                PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                (package, version) =>
                {
                    InstallOptions options = this.GetInstallOptions(version, psPackageInstallMode);
                    if (psProcessorArchitecture != "Default")
                    {
                        options.AllowedArchitectures.Clear();
                        options.AllowedArchitectures.Add(PSEnumHelpers.ToProcessorArchitecture(psProcessorArchitecture));
                    }

                    options.PackageInstallScope = PSEnumHelpers.ToPackageInstallScope(psPackageInstallScope);

                    InstallResult result = this.InstallPackage(package, options);
                    this.PsCmdlet.WriteObject(new PSInstallResult(result));
                });
        }

        /// <summary>
        /// Process update package command.
        /// </summary>
        /// <param name="includeUnknown">If updating to an unknown version is allowed.</param>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageInstallMode">PSPackageInstallMode.</param>
        public void Update(
            bool includeUnknown,
            string psPackageFieldMatchOption,
            string psPackageInstallMode)
        {
            this.GetPackageAndExecute(
                CompositeSearchBehavior.LocalCatalogs,
                PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                (package, version) =>
                {
                    InstallOptions options = this.GetInstallOptions(version, psPackageInstallMode);
                    options.AllowUpgradeToUnknownVersion = includeUnknown;

                    InstallResult result = this.UpgradePackage(package, options);
                    this.PsCmdlet.WriteObject(new PSInstallResult(result));
                });
        }

        private InstallResult InstallPackage(
            CatalogPackage package,
            InstallOptions options)
        {
            var operation = PackageManagerWrapper.Instance.InstallPackageAsync(package, options);
            return this.RegisterCallbacksAndWait(operation, string.Format(
                Resources.ProgressRecordActivityInstalling,
                package.Name));
        }

        private InstallResult UpgradePackage(
            CatalogPackage package,
            InstallOptions options)
        {
            var operation = PackageManagerWrapper.Instance.UpgradePackageAsync(package, options);
            return this.RegisterCallbacksAndWait(
                operation,
                string.Format(
                    Resources.ProgressRecordActivityUpdating,
                    package.Name));
        }
    }
}
