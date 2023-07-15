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
    [OutputType(typeof(int))]
    public class RepairWinGetPackageManagerCmdlet : WinGetPackageManagerCmdlet
    {
        /// <summary>
        /// Gets or sets a value indicating whether to repair for all users. Requires admin.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AllUsers { get; set; }

        /// <summary>
        /// Attempts to repair winget.
        /// TODO: consider WhatIf and Confirm options.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new WinGetPackageManagerCommand(this);
            if (this.ParameterSetName == Constants.IntegrityLatestSet)
            {
                command.RepairUsingLatest(this.IncludePreRelease.ToBool(), this.AllUsers.ToBool());
            }
            else
            {
                command.Repair(this.Version, this.AllUsers.ToBool());
            }
        }
    }
}
