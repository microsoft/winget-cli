// -----------------------------------------------------------------------------
// <copyright file="TestUserSettingCmdlet.cs" company="Microsoft Corporation">
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
    /// Compare the specified user settings with the winget user settings.
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Test, Constants.WinGetNouns.UserSetting)]
    [Alias("twgus", "Test-WinGetUserSettings")]
    [OutputType(typeof(bool))]
    public sealed class TestUserSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the input user settings.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true)]
        public Hashtable UserSettings { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to ignore comparing settings that are not part of the input.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IgnoreNotSet { get; set; }

        /// <summary>
        /// Process the cmdlet and writes the result of the comparison.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new UserSettingsCommand(this);
            command.Test(this.UserSettings, this.IgnoreNotSet.ToBool());
        }
    }
}
