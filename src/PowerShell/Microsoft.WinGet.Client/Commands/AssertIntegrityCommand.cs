// -----------------------------------------------------------------------------
// <copyright file="AssertIntegrityCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Assert-WinGetIntegrity. Verifies winget is installed properly.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Assert, Constants.WinGetNouns.Integrity)]
    public class AssertIntegrityCommand : BaseCommand
    {
        /// <summary>
        /// Validates winget is installed correctly. If not, throws an exception
        /// with the reason why, if any.
        /// </summary>
        protected override void ProcessRecord()
        {
            WinGetIntegrity.AssertWinGet(this.InvokeCommand);
        }
    }
}
