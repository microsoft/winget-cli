// -----------------------------------------------------------------------------
// <copyright file="SourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Properties;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Wrapper for source cmdlets.
    /// </summary>
    public sealed class SourceCommand : ClientCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceCommand"/> class.
        /// Wrapper for Source commands.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        public SourceCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
#if POWERSHELL_WINDOWS
            throw new NotSupportedException(Resources.WindowsPowerShellNotSupported);
#endif
        }

        /// <summary>
        /// Get-WinGetSource.
        /// </summary>
        /// <param name="name">Optional name.</param>
        public void Get(string name)
        {
            var results = GetPackageCatalogReferences(name);
            for (var i = 0; i < results.Count; i++)
            {
                this.PsCmdlet.WriteObject(new PSSourceResult(results[i]));
            }
        }
    }
}
