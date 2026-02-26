// -----------------------------------------------------------------------------
// <copyright file="CompleteWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Complete-WinGetConfiguration.
    /// Completes a configuration previously started by Start-WinGetConfiguration.
    /// Waits for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Complete, "WinGetConfiguration")]
    [Alias("cmpwgc")]
    public sealed class CompleteWinGetConfigurationCmdlet : PSCmdlet
    {
        private ConfigurationCommand runningCommand = null;

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
            this.runningCommand = new ConfigurationCommand(this);
            this.runningCommand.Continue(this.ConfigurationJob);
        }

        /// <summary>
        /// Interrupts currently running code within the command.
        /// </summary>
        protected override void StopProcessing()
        {
            if (this.runningCommand != null)
            {
                this.runningCommand.Cancel(this.ConfigurationJob);
                this.runningCommand.Cancel();
            }
        }
    }
}
