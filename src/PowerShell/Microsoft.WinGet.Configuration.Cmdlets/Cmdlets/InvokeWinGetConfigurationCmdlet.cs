// -----------------------------------------------------------------------------
// <copyright file="InvokeWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using System.Threading;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Helpers;

    /// <summary>
    /// Invoke-WinGetConfiguration.
    /// Start configuration and waits for completion.
    /// </summary>
    [Cmdlet(VerbsLifecycle.Invoke, "WinGetConfiguration")]
    public sealed class InvokeWinGetConfigurationCmdlet : PSCmdlet
    {
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
            // The cmdlet doesn't inherit the location from the current session.
            // Change it to support relative paths.
            Utilities.ChangeToCurrentSessionLocation();
        }

        /// <summary>
        /// Starts configuration and wait for it to complete.
        /// </summary>
        protected override void ProcessRecord()
        {
            CancellationTokenSource source = new ();
            var configCommand = new ConfigurationCommand(this, source.Token, this.File);

            configCommand.Invoke();
        }
    }
}
