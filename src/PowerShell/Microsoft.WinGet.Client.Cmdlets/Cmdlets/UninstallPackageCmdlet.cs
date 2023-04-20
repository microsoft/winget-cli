// -----------------------------------------------------------------------------
// <copyright file="UninstallPackageCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Client.PSObjects;

    /// <summary>
    /// Uninstalls a package from the local system.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Uninstall,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSUninstallResult))]
    public sealed class UninstallPackageCmdlet : PackageCmdlet
    {
        /// <summary>
        /// Gets or sets the desired mode for the uninstallation process.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageUninstallMode Mode { get; set; } = PSPackageUninstallMode.Default;

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Uninstalls a package from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new UninstallPackageCommand(
                this,
                this.PSCatalogPackage,
                this.Version,
                this.Log,
                this.Id,
                this.Name,
                this.Moniker,
                this.Source,
                this.Query,
                this.MatchOption.ToString());
            command.Uninstall(this.Mode.ToString(), this.Force.ToBool());
        }
    }
}
