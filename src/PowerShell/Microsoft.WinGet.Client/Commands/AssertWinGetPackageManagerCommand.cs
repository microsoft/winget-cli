// -----------------------------------------------------------------------------
// <copyright file="AssertWinGetPackageManagerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Assert-WinGetPackageManager. Verifies winget is installed properly.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Assert,
        Constants.WinGetNouns.WinGetPackageManager,
        DefaultParameterSetName = Constants.IntegrityVersionSet)]
    public class AssertWinGetPackageManagerCommand : WinGetPackageManagerCommand
    {
        /// <summary>
        /// Validates winget is installed correctly. If not, throws an exception
        /// with the reason why, if any.
        /// </summary>
        protected override void ProcessRecord()
        {
            if (this.ParameterSetName == Constants.IntegrityLatestSet)
            {
                // TODO: call WinGetPackageManager AssertUsingLatest(this, this.IncludePreRelease.ToBool())
            }
            else
            {
                // TODO: call WinGetPackageManager Assert(this, this.Version)
            }
        }
    }
}
