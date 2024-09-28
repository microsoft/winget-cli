// -----------------------------------------------------------------------------
// <copyright file="RepairPackageCmdlet.cs" company="Microsoft Corporation">
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
    /// This class defines the repair package command.
    /// </summary>
    [Cmdlet(
        VerbsDiagnostic.Repair,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSRepairResult))]
    public sealed class RepairPackageCmdlet : PackageCmdlet
    {
        private RepairPackageCommand command = null;

        /// <summary>
        /// Gets or sets the desired mode for the repair process.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageRepairMode Mode { get; set; } = PSPackageRepairMode.Default;

        /// <summary>
        /// Gets or sets the path to the logging file.
        /// </summary>
        [Parameter]
        public string Log { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Repairs a package from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new RepairPackageCommand(
                        this,
                        this.AllowHashMismatch.ToBool(),
                        this.Force.ToBool(),
                        this.PSCatalogPackage,
                        this.Version,
                        this.Log,
                        this.Id,
                        this.Name,
                        this.Moniker,
                        this.Source,
                        this.Query);
            this.command.Repair(this.MatchOption.ToString(), this.Mode.ToString());
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
