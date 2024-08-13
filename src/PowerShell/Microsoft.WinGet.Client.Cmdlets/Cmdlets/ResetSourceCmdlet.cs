// -----------------------------------------------------------------------------
// <copyright file="ResetSourceCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Resets a source. Requires admin.
    /// </summary>
    [Cmdlet(VerbsCommon.Reset, Constants.WinGetNouns.Source)]
    public sealed class ResetSourceCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the source to reset.
        /// </summary>
        [Parameter(
            Mandatory = false,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Resets source.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);

            if (!string.IsNullOrEmpty(this.Name))
            {
                command.ResetSourceByName(this.Name);
            }
            else
            {
                command.ResetSources();
            }
        }
    }
}
