// -----------------------------------------------------------------------------
// <copyright file="VersionCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;

    /// <summary>
    /// Version commands.
    /// </summary>
    internal class VersionCommand : BaseCommand
    {
        private readonly PSCmdlet psCmdlet;

        /// <summary>
        /// Initializes a new instance of the <see cref="VersionCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">The caller cmdlet.</param>
        public VersionCommand(PSCmdlet psCmdlet)
        {
            this.psCmdlet = psCmdlet;
        }

        /// <summary>
        /// Get-WinGetVersion. Gets the currently installed winget version.
        /// </summary>
        public void Get()
        {
            this.psCmdlet.WriteObject(WinGetVersion.InstalledWinGetVersion.TagVersion);
        }
    }
}
