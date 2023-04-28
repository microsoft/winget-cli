// -----------------------------------------------------------------------------
// <copyright file="GetWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using System.Threading;
    using Microsoft.PowerShell;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Helpers;

    /// <summary>
    /// Get-WinGetConfigurationSet.
    /// Start configuration and waits for completion.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "WinGetConfiguration")]
    public sealed class GetWinGetConfigurationCmdlet : PSCmdlet
    {
        private ExecutionPolicy executionPolicy = ExecutionPolicy.Undefined;

        /// <summary>
        /// Gets or sets the configuration file.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true)]
        public string File { get; set; }

        /// <summary>
        /// Pre-processing operations.
        /// </summary>
        protected override void BeginProcessing()
        {
            this.executionPolicy = Utilities.GetExecutionPolicy();
        }

        /// <summary>
        /// Opens the configuration set.
        /// </summary>
        protected override void ProcessRecord()
        {
            CancellationTokenSource source = new ();

            var configCommand = new ConfigurationCommand(this, source.Token);
            configCommand.Get(this.File, this.executionPolicy);
        }
    }
}
