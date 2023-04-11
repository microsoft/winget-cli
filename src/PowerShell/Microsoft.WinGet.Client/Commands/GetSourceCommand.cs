// -----------------------------------------------------------------------------
// <copyright file="GetSourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Retrieves the list of configured sources.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Source)]
    [OutputType(typeof(PSObjects.PSSourceResult))]
    public sealed class GetSourceCommand : PSCmdlet
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
            // TODO: call SourceCommand.Get
        }
    }
}
