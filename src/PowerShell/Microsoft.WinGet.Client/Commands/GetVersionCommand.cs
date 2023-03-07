// -----------------------------------------------------------------------------
// <copyright file="GetVersionCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Helpers;

    /// <summary>
    /// Get-WinGetVersion. Gets the current version of winget.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Version)]
    [OutputType(typeof(string))]
    public class GetVersionCommand : BaseCommand
    {
        /// <summary>
        /// Writes the winget version.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.WriteObject(WinGetVersion.InstalledWinGetVersion.TagVersion);
        }
    }
}
