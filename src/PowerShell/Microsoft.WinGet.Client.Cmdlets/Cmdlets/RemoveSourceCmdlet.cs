// -----------------------------------------------------------------------------
// <copyright file="RemoveSourceCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Removes a source. Requires admin.
    /// </summary>
    [Cmdlet(VerbsCommon.Remove, Constants.WinGetNouns.Source)]
    [Alias("rwgs")]
    public sealed class RemoveSourceCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the source to remove.
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Removes source.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.RemoveSource(this.Name);
        }
    }
}
