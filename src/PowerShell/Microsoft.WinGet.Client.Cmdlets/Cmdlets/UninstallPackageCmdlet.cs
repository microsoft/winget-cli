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
    [Alias("uswgp")]
    [OutputType(typeof(PSUninstallResult))]
    public sealed class UninstallPackageCmdlet : PackageCmdlet
    {
        private UninstallPackageCommand command = null;

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
        /// Gets or sets the path to the logging file.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Log { get; set; }

        /// <summary>
        /// Uninstalls a package from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new UninstallPackageCommand(
                        this,
                        this.PSCatalogPackage,
                        this.Version,
                        this.Log,
                        this.Id,
                        this.Name,
                        this.Moniker,
                        this.Source,
                        this.Query);
            this.command.Uninstall(this.MatchOption.ToString(), this.Mode.ToString(), this.Force.ToBool());
        }

        /// <summary>
        /// Interrupts currently running code within the command.
        /// </summary>
        protected override void StopProcessing()
        {
            if (this.command != null)
            {
                this.command.Cancel();
            }
        }
    }
}
