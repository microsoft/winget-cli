// -----------------------------------------------------------------------------
// <copyright file="ConvertToWinGetConfigurationYamlCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// ConvertTo-WinGetConfigurationYaml
    /// Serializes a PSConfigurationSet to a YAML string.
    /// </summary>
    [Cmdlet(VerbsData.ConvertTo, "WinGetConfigurationYaml")]
    [Alias("ctwgcy")]
    public sealed class ConvertToWinGetConfigurationYamlCmdlet : PSCmdlet
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
        /// Converts the given set to a string.
        /// </summary>
        protected override void ProcessRecord()
        {
            var configCommand = new ConfigurationCommand(this);
            configCommand.Serialize(this.Set);
        }
    }
}
