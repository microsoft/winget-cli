// -----------------------------------------------------------------------------
// <copyright file="UpdatePackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// This commands updates a package from the pipeline or from the local system.
    /// </summary>
    [Cmdlet(
        VerbsData.Update,
        Constants.PackageNoun,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(InstallResult))]
    public sealed class UpdatePackageCommand : BaseInstallCommand
    {
        /// <summary>
        /// Gets or sets a value indicating whether updating to an unknown version is allowed.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IncludeUnknown { get; set; }

        /// <summary>
        /// Updates a package from the pipeline or from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            base.ProcessRecord();
            this.GetPackageAndExecute(CompositeSearchBehavior.LocalCatalogs, (package, version) =>
            {
                InstallOptions options = this.GetInstallOptions(version);
                InstallResult result = this.UpgradePackage(package, options);
                this.WriteObject(result);
            });
        }

        /// <inheritdoc />
        protected override InstallOptions GetInstallOptions(PackageVersionId version)
        {
            InstallOptions options = base.GetInstallOptions(version);
            options.AllowUpgradeToUnknownVersion = this.IncludeUnknown.ToBool();
            return options;
        }

        private InstallResult UpgradePackage(
            CatalogPackage package,
            InstallOptions options)
        {
            var operation = PackageManager.Value.UpgradePackageAsync(package, options);
            return this.RegisterCallbacksAndWait(operation, string.Format(
                Utilities.ResourceManager.GetString("ProgressRecordActivityUpdating"),
                package.Name));
        }
    }
}
