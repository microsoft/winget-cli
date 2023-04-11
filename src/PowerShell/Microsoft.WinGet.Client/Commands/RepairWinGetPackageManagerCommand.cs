// -----------------------------------------------------------------------------
// <copyright file="RepairWinGetPackageManagerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Helpers;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Repair-WinGetPackageManager. Repairs winget if needed.
    /// </summary>
    [Cmdlet(
        VerbsDiagnostic.Repair,
        Constants.WinGetNouns.WinGetPackageManager,
        DefaultParameterSetName = Constants.IntegrityVersionSet)]
    [OutputType(typeof(int))]
    public class RepairWinGetPackageManagerCommand : WinGetPackageManagerCommand
    {
        /// <summary>
        /// Attempts to repair winget.
        /// TODO: consider WhatIf and Confirm options.
        /// </summary>
        protected override void ProcessRecord()
        {
            string expectedVersion = this.Version;
            if (this.ParameterSetName == Constants.IntegrityLatestSet)
            {
                // TODO: call WinGetPackageManagerCommand RepairUsingLatest(this, this.IncludePreRelease.ToBool())
            }
            else
            {
                // TODO: call WinGetPackageManagerCommand Repair(this, this.Version)
            }
        }
    }
}
