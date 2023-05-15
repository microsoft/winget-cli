// -----------------------------------------------------------------------------
// <copyright file="FinderPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.Properties;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Searches configured sources for packages.
    /// </summary>
    public sealed class FinderPackageCommand : FinderExtendedCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FinderPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        /// <param name="matchOption">Match option.</param>
        /// <param name="tag">Tag of the package.</param>
        /// <param name="command">Command of the package.</param>
        /// <param name="count">Max results to return.</param>
        public FinderPackageCommand(
            PSCmdlet psCmdlet,
            string id,
            string name,
            string moniker,
            string source,
            string[] query,
            string matchOption,
            string tag,
            string command,
            uint count)
            : base(psCmdlet)
        {
#if POWERSHELL_WINDOWS
            throw new NotSupportedException(Resources.WindowsPowerShellNotSupported);
#else
            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;
            this.MatchOption = PSEnumHelpers.ToPackageFieldMatchOption(matchOption);

            // FinderExtendedCommand
            this.Tag = tag;
            this.Command = command;
            this.Count = count;
#endif
        }

        /// <summary>
        /// Process find package command.
        /// </summary>
        public void Find()
        {
            var results = this.FindPackages(CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs);
            for (var i = 0; i < results.Count; i++)
            {
                this.PsCmdlet.WriteObject(new PSFoundCatalogPackage(results[i].CatalogPackage));
            }
        }

        /// <summary>
        /// Process get package command.
        /// </summary>
        public void Get()
        {
            var results = this.FindPackages(CompositeSearchBehavior.LocalCatalogs);
            for (var i = 0; i < results.Count; i++)
            {
                this.PsCmdlet.WriteObject(new PSInstalledCatalogPackage(results[i].CatalogPackage));
            }
        }
    }
}
