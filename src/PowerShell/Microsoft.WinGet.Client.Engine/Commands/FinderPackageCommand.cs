// -----------------------------------------------------------------------------
// <copyright file="FinderPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;

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
            string tag,
            string command,
            uint count)
            : base(psCmdlet)
        {
            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;

            // FinderExtendedCommand
            this.Tag = tag;
            this.Command = command;
            this.Count = count;
        }

        /// <summary>
        /// Process find package command.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        public void Find(string psPackageFieldMatchOption)
        {
            var results = this.Execute(
                () => this.FindPackages(
                    CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption)));

            for (var i = 0; i < results.Count; i++)
            {
                this.Write(StreamType.Object, new PSFoundCatalogPackage(results[i].CatalogPackage));
            }
        }

        /// <summary>
        /// Process get package command.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        public void Get(string psPackageFieldMatchOption)
        {
            var results = this.Execute(
                () => this.FindPackages(
                    CompositeSearchBehavior.LocalCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption)));
            for (var i = 0; i < results.Count; i++)
            {
                this.Write(StreamType.Object, new PSInstalledCatalogPackage(results[i].CatalogPackage));
            }
        }
    }
}
