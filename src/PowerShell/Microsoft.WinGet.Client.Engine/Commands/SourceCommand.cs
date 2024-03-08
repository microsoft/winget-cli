// -----------------------------------------------------------------------------
// <copyright file="SourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Wrapper for source cmdlets.
    /// </summary>
    public sealed class SourceCommand : ManagementDeploymentCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceCommand"/> class.
        /// Wrapper for Source commands.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        public SourceCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Get-WinGetSource.
        /// </summary>
        /// <param name="name">Optional name.</param>
        public void Get(string name)
        {
            var results = this.Execute(
                () => this.GetPackageCatalogReferences(name));
            for (var i = 0; i < results.Count; i++)
            {
                this.Write(StreamType.Object, new PSSourceResult(results[i]));
            }
        }
    }
}
