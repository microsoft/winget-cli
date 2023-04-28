// -----------------------------------------------------------------------------
// <copyright file="StartWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using System.Threading;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Start-WinGetConfigurationSet.
    /// Start configuration and waits for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Start, "WinGetConfiguration")]
    public sealed class StartWinGetConfigurationCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the configuration set.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSConfigurationSet Set { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to accept the configuration agreements.
        /// </summary>
        public SwitchParameter AcceptConfigurationAgreements { get; set; }

        /// <summary>
        /// Starts configuration and wait for it to complete.
        /// </summary>
        protected override void ProcessRecord()
        {
            CancellationTokenSource source = new ();

            var configCommand = new ConfigurationCommand(this, source.Token);
            configCommand.Start(this.Set, this.AcceptConfigurationAgreements.ToBool());
        }
    }
}
