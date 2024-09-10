// -----------------------------------------------------------------------------
// <copyright file="TestWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Test-WinGetConfiguration
    /// Tests configuration.
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Test, "WinGetConfiguration")]
    [Alias("twgc")]
    public class TestWinGetConfigurationCmdlet : PSCmdlet
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
            this.acceptedAgreements = ConfigurationCommand.ConfirmConfigurationProcessing(this, this.AcceptConfigurationAgreements.ToBool(), false);
        }

        /// <summary>
        /// Test configuration.
        /// </summary>
        protected override void ProcessRecord()
        {
            if (this.acceptedAgreements)
            {
                this.runningCommand = new ConfigurationCommand(this);
                this.runningCommand.Test(this.Set);
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
