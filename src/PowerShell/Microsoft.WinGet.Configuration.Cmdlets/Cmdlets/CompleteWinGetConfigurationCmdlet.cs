// -----------------------------------------------------------------------------
// <copyright file="CompleteWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
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
    /// Complete-WinGetConfiguration.
    /// Completes a configuration previously started by Start-WinGetConfiguration.
    /// Waits for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Complete, "WinGetConfiguration")]
    public sealed class CompleteWinGetConfigurationCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the configuration task.
        /// </summary>
        [Parameter(
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
            configCommand.Continue(this.ConfigurationJob);
        }
    }
}
