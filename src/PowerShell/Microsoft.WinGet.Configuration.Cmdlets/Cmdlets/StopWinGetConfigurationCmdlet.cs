// -----------------------------------------------------------------------------
// <copyright file="StopWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Stop-WinGetConfiguration.
    /// Cancels a configuration previously started by Start-WinGetConfiguration.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Stop, "WinGetConfiguration")]
    [Alias("spwgc")]
    public sealed class StopWinGetConfigurationCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the configuration task.
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSConfigurationJob ConfigurationJob { get; set; }

        /// <summary>
        /// Starts to apply the configuration and wait for it to complete.
        /// </summary>
        protected override void ProcessRecord()
        {
            var configCommand = new ConfigurationCommand(this);
            configCommand.Cancel(this.ConfigurationJob);
        }
    }
}
