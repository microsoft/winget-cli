// -----------------------------------------------------------------------------
// <copyright file="AddSourceCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Adds a source. Requires admin.
    /// </summary>
    [Cmdlet(VerbsCommon.Add, Constants.WinGetNouns.Source)]
    public sealed class AddSourceCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the source to add.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the argument of the source to add.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Argument { get; set; }

        /// <summary>
        /// Gets or sets the type of the source to add.
        /// </summary>
        [Parameter(
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Type { get; set; }

        /// <summary>
        /// Adds source.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.AddSource(this.Name, this.Argument, this.Type);
        }
    }
}
