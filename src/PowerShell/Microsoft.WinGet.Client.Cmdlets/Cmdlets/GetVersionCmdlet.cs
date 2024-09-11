// -----------------------------------------------------------------------------
// <copyright file="GetVersionCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Get-WinGetVersion. Gets the current version of winget.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Version)]
    [Alias("gwgv")]
    [OutputType(typeof(string))]
    public class GetVersionCmdlet : PSCmdlet
    {
        /// <summary>
        /// Writes the winget version.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new VersionCommand(this);
            command.Get();
        }
    }
}
