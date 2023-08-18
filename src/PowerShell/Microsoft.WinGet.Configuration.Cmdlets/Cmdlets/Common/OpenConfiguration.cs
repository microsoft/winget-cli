// -----------------------------------------------------------------------------
// <copyright file="OpenConfiguration.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets.Common
{
    using System.Management.Automation;
    using Microsoft.PowerShell;
    using Microsoft.WinGet.Configuration.Helpers;

    /// <summary>
    /// The parameters used to open a configuration.
    /// </summary>
    public abstract class OpenConfiguration : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the configuration file.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = Constants.ParameterSet.OpenConfigurationSet)]
        public string File { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to use the all users location to install modules.
        /// </summary>
        [Parameter(
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = Constants.ParameterSet.OpenConfigurationSet)]
        public SwitchParameter AllUsers { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to use the current user location to install modules.
        /// </summary>
        [Parameter(
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = Constants.ParameterSet.OpenConfigurationSet)]
        public SwitchParameter CurrentUser { get; set; }

        /// <summary>
        /// Gets or sets custom location to install modules.
        /// </summary>
        [Parameter(
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = Constants.ParameterSet.OpenConfigurationSet)]
        public string CustomLocation { get; set; }

        /// <summary>
        /// Gets the execution policy to use.
        /// </summary>
        protected ExecutionPolicy ExecutionPolicy { get; private set; } = ExecutionPolicy.Undefined;

        /// <summary>
        /// Gets a value indicating whether to use telemetry or not.
        /// </summary>
        protected bool CanUseTelemetry { get; private set; }

        /// <summary>
        /// Pre-processing operations.
        /// </summary>
        protected override void BeginProcessing()
        {
            this.ExecutionPolicy = Utilities.GetExecutionPolicy();
            this.CanUseTelemetry = Utilities.CanUseTelemetry();
        }
    }
}
