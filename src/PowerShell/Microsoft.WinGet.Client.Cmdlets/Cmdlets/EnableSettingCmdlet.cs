// -----------------------------------------------------------------------------
// <copyright file="EnableSettingCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Enables an admin setting. Requires admin.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Enable, Constants.WinGetNouns.Setting)]
    [Alias("ewgs")]
    public sealed class EnableSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the setting to enable.
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        [ValidateSet(
            "LocalManifestFiles",
            "BypassCertificatePinningForMicrosoftStore",
            "InstallerHashOverride",
            "LocalArchiveMalwareScanOverride",
            "ProxyCommandLineOptions")]
        public string Name { get; set; }

        /// <summary>
        /// Enables the admin setting.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.EnableSetting(this.Name);
        }
    }
}
