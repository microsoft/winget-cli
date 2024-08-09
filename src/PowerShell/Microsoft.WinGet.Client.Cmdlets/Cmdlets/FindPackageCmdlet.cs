// -----------------------------------------------------------------------------
// <copyright file="FindPackageCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Searches configured sources for packages.
    /// </summary>
    [Cmdlet(VerbsCommon.Find, Constants.WinGetNouns.Package)]
    [Alias("fdwgp")]
    [OutputType(typeof(PSFoundCatalogPackage))]
    public sealed class FindPackageCmdlet : FinderExtendedCmdlet
    {
        /// <summary>
        /// Searches for configured sources for packages.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new FinderPackageCommand(
                this,
                this.Id,
                this.Name,
                this.Moniker,
                this.Source,
                this.Query,
                this.Tag,
                this.Command,
                this.Count);

            command.Find(this.MatchOption.ToString());
        }
    }
}
