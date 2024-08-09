// -----------------------------------------------------------------------------
// <copyright file="GetSourceCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Retrieves the list of configured sources.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Source)]
    [Alias("gwgso")]
    [OutputType(typeof(PSSourceResult))]
    public sealed class GetSourceCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the source to retrieve.
        /// </summary>
        [Parameter(
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Returns the list of configured sources.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new SourceCommand(this);
            command.Get(this.Name);
        }
    }
}
