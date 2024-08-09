// -----------------------------------------------------------------------------
// <copyright file="InvokeWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Invoke-WinGetConfiguration.
    /// Applies the configuration.
    /// Wait for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Invoke, "WinGetConfiguration")]
    [Alias("iwgc")]
    public sealed class InvokeWinGetConfigurationCmdlet : PSCmdlet
    {
        private bool acceptedAgreements = false;
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
        /// Gets or sets a value indicating whether to accept the configuration agreements.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AcceptConfigurationAgreements { get; set; }

        /// <summary>
        /// Pre-processing operations.
        /// </summary>
        protected override void BeginProcessing()
        {
            this.acceptedAgreements = ConfigurationCommand.ConfirmConfigurationProcessing(this, this.AcceptConfigurationAgreements.ToBool(), true);
        }

        /// <summary>
        /// Starts to apply the configuration and wait for it to complete.
        /// </summary>
        protected override void ProcessRecord()
        {
            if (this.acceptedAgreements)
            {
                this.runningCommand = new ConfigurationCommand(this);
                this.runningCommand.Apply(this.Set);
            }
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
