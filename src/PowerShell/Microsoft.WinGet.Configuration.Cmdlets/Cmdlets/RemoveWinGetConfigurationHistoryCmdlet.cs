// -----------------------------------------------------------------------------
// <copyright file="RemoveWinGetConfigurationHistoryCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Remove-WinGetConfigurationHistory.
    /// Removes the given configuration set from history.
    /// </summary>
    [Cmdlet(VerbsCommon.Remove, "WinGetConfigurationHistory")]
    [Alias("rwgch")]
    public sealed class RemoveWinGetConfigurationHistoryCmdlet : PSCmdlet
    {
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
        /// Removes the given set from history.
        /// </summary>
        protected override void ProcessRecord()
        {
            var configCommand = new ConfigurationCommand(this);
            configCommand.Remove(this.Set);
        }
    }
}
