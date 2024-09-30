// -----------------------------------------------------------------------------
// <copyright file="ConfirmWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Confirm-WinGetConfiguration
    /// Validates winget configuration.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Confirm, "WinGetConfiguration")]
    [Alias("cnwgc")]
    public class ConfirmWinGetConfigurationCmdlet : PSCmdlet
    {
        private ConfigurationCommand runningCommand = null;

        /// <summary>
        /// Gets or sets the configuration set.
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSConfigurationSet Set { get; set; }

        /// <summary>
        /// Validate configuration.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.runningCommand = new ConfigurationCommand(this);
            this.runningCommand.Validate(this.Set);
        }

        /// <summary>
        /// Interrupts currently running code within the command.
        /// </summary>
        protected override void StopProcessing()
        {
            if (this.runningCommand != null)
            {
                this.runningCommand.Cancel();
            }
        }
    }
}
