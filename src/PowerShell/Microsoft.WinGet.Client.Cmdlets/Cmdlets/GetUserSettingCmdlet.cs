// -----------------------------------------------------------------------------
// <copyright file="GetUserSettingCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Collections;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Gets winget's user settings.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.UserSetting)]
    [Alias("gwgus", "Get-WinGetUserSettings")]
    [OutputType(typeof(Hashtable))]
    public sealed class GetUserSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets the settings file contents.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new UserSettingsCommand(this);
            command.Get();
        }
    }
}
