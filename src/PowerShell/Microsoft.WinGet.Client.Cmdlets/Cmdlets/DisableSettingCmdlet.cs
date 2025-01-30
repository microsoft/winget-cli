// -----------------------------------------------------------------------------
// <copyright file="DisableSettingCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Disables an admin setting. Requires admin.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Disable, Constants.WinGetNouns.Setting)]
    [Alias("dwgs")]
    public sealed class DisableSettingCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the setting to disable.
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
        /// Disables the admin setting.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.DisableSetting(this.Name);
        }
    }
}
