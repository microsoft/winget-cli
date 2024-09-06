// -----------------------------------------------------------------------------
// <copyright file="GetSettingCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Gets winget settings.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Setting)]
    [Alias("gwgse", "Get-WinGetSettings")]
    public sealed class GetSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets a value indicating whether to output a string or a hashtable.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AsPlainText { get; set; }

        /// <summary>
        /// Get settings.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.GetSettings(this.AsPlainText.ToBool());
        }
    }
}
