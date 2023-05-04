// -----------------------------------------------------------------------------
// <copyright file="GetWinGetConfigurationDetailsCmdlet.cs" company="Microsoft Corporation">
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
    /// Get-WinGetConfigurationDetails.
    /// Gets the details for the units in a configuration set.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "WinGetConfigurationDetails")]
    public sealed class GetWinGetConfigurationDetailsCmdlet : PSCmdlet
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
        /// Starts configuration and wait for it to complete.
        /// </summary>
        protected override void ProcessRecord()
        {
            CancellationTokenSource source = new ();

            var configCommand = new ConfigurationCommand(this);
            configCommand.GetDetails(this.Set);
        }
    }
}
