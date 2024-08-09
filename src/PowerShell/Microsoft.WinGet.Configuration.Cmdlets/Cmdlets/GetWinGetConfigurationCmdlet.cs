// -----------------------------------------------------------------------------
// <copyright file="GetWinGetConfigurationCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Cmdlets.Common;
    using Microsoft.WinGet.Configuration.Engine.Commands;

    /// <summary>
    /// Get-WinGetConfiguration.
    /// Opens a configuration set.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "WinGetConfiguration", DefaultParameterSetName = Helpers.Constants.ParameterSet.OpenConfigurationSetFromFile)]
    [Alias("gwgc")]
    public sealed class GetWinGetConfigurationCmdlet : OpenConfiguration
    {
        /// <summary>
        /// Opens the configuration set.
        /// </summary>
        protected override void ProcessRecord()
        {
            var configCommand = new ConfigurationCommand(this);

            if (this.ParameterSetName == Helpers.Constants.ParameterSet.OpenConfigurationSetFromFile)
            {
                configCommand.Get(
                    this.File,
                    this.ModulePath,
                    this.ExecutionPolicy,
                    this.CanUseTelemetry);
            }
            else if (this.ParameterSetName == Helpers.Constants.ParameterSet.OpenConfigurationSetFromHistory)
            {
                configCommand.GetFromHistory(
                    this.InstanceIdentifier,
                    this.ModulePath,
                    this.ExecutionPolicy,
                    this.CanUseTelemetry);
            }
            else if (this.ParameterSetName == Helpers.Constants.ParameterSet.OpenAllConfigurationSetsFromHistory)
            {
                configCommand.GetAllFromHistory(
                    this.ModulePath,
                    this.ExecutionPolicy,
                    this.CanUseTelemetry);
            }
        }
    }
}
