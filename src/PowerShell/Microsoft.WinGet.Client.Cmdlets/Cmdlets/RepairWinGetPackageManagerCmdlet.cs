// -----------------------------------------------------------------------------
// <copyright file="RepairWinGetPackageManagerCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Repair-WinGetPackageManager. Repairs winget if needed.
    /// </summary>
    [Cmdlet(
        VerbsDiagnostic.Repair,
        Constants.WinGetNouns.WinGetPackageManager,
        DefaultParameterSetName = Constants.IntegrityVersionSet)]
    [Alias("rpwgpm")]
    [OutputType(typeof(int))]
    public class RepairWinGetPackageManagerCmdlet : WinGetPackageManagerCmdlet
    {
        private WinGetPackageManagerCommand command = null;

        /// <summary>
        /// Gets or sets a value indicating whether to repair for all users. Requires admin.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AllUsers { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to force application shutdown.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Attempts to repair winget.
        /// TODO: consider WhatIf and Confirm options.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new WinGetPackageManagerCommand(this);
            if (this.ParameterSetName == Constants.IntegrityLatestSet)
            {
                this.command.RepairUsingLatest(this.IncludePrerelease.ToBool(), this.AllUsers.ToBool(), this.Force.ToBool());
            }
            else
            {
                this.command.Repair(this.Version, this.AllUsers.ToBool(), this.Force.ToBool());
            }
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
