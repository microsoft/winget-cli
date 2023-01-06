// -----------------------------------------------------------------------------
// <copyright file="GetUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Collections;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Gets winget's user settings.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.UserSettings)]
    [OutputType(typeof(Hashtable))]
    public sealed class GetUserSettingsCommand : BaseUserSettingsCommand
    {
        /// <summary>
        /// Writes the settings file contents.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.WriteObject(this.GetLocalSettingsAsHashtable());
        }
    }
}
