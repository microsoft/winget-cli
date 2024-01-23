// -----------------------------------------------------------------------------
// <copyright file="GetWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Cmdlets.Common;
    using Microsoft.WinGet.Configuration.Engine.Commands;

    /// <summary>
    /// Get-WinGetConfiguration.
    /// Opens a configuration set.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "WinGetConfiguration")]
    public sealed class GetWinGetConfigurationCmdlet : OpenConfiguration
    {
        /// <summary>
        /// Opens the configuration set.
        /// </summary>
        protected override void ProcessRecord()
        {
            var configCommand = new ConfigurationCommand(this);
            configCommand.Get(
                this.File,
                this.ModulePath,
                this.ExecutionPolicy,
                this.CanUseTelemetry);
        }
    }
}
