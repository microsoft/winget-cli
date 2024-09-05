// -----------------------------------------------------------------------------
// <copyright file="SetUserSettingCmdlet.cs" company="Microsoft Corporation">
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
    /// Sets the specified user settings into the winget user settings. If the merge switch is on, merges current user
    /// settings with the input settings. Otherwise, overwrites the input settings.
    /// </summary>
    [Cmdlet(VerbsCommon.Set, Constants.WinGetNouns.UserSetting)]
    [Alias("swgus", "Set-WinGetUserSettings")]
    [OutputType(typeof(Hashtable))]
    public sealed class SetUserSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the input user settings.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true)]
        public Hashtable UserSettings { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to merge the current user settings and the input settings.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Merge { get; set; }

        /// <summary>
        /// Process input of cmdlet.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new UserSettingsCommand(this);
            command.Set(this.UserSettings, this.Merge.ToBool());
        }
    }
}
