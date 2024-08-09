// -----------------------------------------------------------------------------
// <copyright file="StartWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Start-WinGetConfiguration.
    /// Start to apply the configuration.
    /// Does not wait for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Start, "WinGetConfiguration")]
    [Alias("sawgc")]
    public sealed class StartWinGetConfigurationCmdlet : PSCmdlet
    {
        private bool acceptedAgreements = false;

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
                var configCommand = new ConfigurationCommand(this);
                configCommand.StartApply(this.Set);
            }
        }
    }
}
