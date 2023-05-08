// -----------------------------------------------------------------------------
// <copyright file="PackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Extensions;

    /// <summary>
    /// This is the base class for commands which operate on a specific package and version i.e.,
    /// the "install", "uninstall", and "upgrade" commands.
    /// </summary>
    public abstract class PackageCommand : FinderCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal PackageCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets the package to directly install.
        /// </summary>
        /// <remarks>
        /// Must match the name of the <see cref="CatalogPackage" /> field on the <see cref="MatchResult" /> class.
        /// </remarks>
        protected CatalogPackage CatalogPackage { get; set; } = null;

        /// <summary>
        /// Gets or sets the version to install.
        /// </summary>
        protected string Version { get; set; }

        /// <summary>
        /// Gets or sets the path to the logging file.
        /// </summary>
        protected string Log { get; set; }

        /// <summary>
        /// Executes a command targeting a specific package version.
        /// </summary>
        /// <param name="behavior">The <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="callback">The method to call after retrieving the package and version to operate upon.</param>
        protected void GetPackageAndExecute(
            CompositeSearchBehavior behavior,
            Action<CatalogPackage, PackageVersionId> callback)
        {
            CatalogPackage package = this.GetCatalogPackage(behavior);
            PackageVersionId version = this.GetPackageVersionId(package);
            if (this.PsCmdlet.ShouldProcess(package.ToString(version)))
            {
                callback(package, version);
            }
        }

        /// <summary>
        /// Sets the find package options for a query input that is looking for a specific package.
        /// </summary>
        /// <param name="options">The options object.</param>
        /// <param name="match">The match type.</param>
        /// <param name="value">The query value.</param>
        protected override void SetQueryInFindPackagesOptions(
            ref FindPackagesOptions options,
            PackageFieldMatchOption match,
            string value)
        {
            foreach (PackageMatchField field in new PackageMatchField[] { PackageMatchField.Id, PackageMatchField.Name, PackageMatchField.Moniker })
            {
                var selector = ComObjectFactory.Value.CreatePackageMatchFilter();
                selector.Field = field;
                selector.Value = value ?? string.Empty;
                selector.Option = match;
                options.Selectors.Add(selector);
            }
        }

        private CatalogPackage GetCatalogPackage(CompositeSearchBehavior behavior)
        {
            if (this.CatalogPackage != null)
            {
                // The package was already provided via a parameter or the pipeline.
                return this.CatalogPackage;
            }
            else
            {
                IReadOnlyList<MatchResult> results = this.FindPackages(behavior, 0);
                if (results.Count == 1)
                {
                    // Exactly one package matched, so we can just return it.
                    return results[0].CatalogPackage;
                }
                else if (results.Count == 0)
                {
                    // No packages matched, we need to throw an error.
                    throw new NoPackageFoundException();
                }
                else
                {
                    // Too many packages matched! The user needs to refine their input.
                    throw new VagueCriteriaException(results);
                }
            }
        }

        private PackageVersionId GetPackageVersionId(CatalogPackage package)
        {
            if (this.Version != null)
            {
                for (var i = 0; i < package.AvailableVersions.Count; i++)
                {
                    if (package.AvailableVersions[i].Version.CompareTo(this.Version) == 0)
                    {
                        return package.AvailableVersions[i];
                    }
                }

                throw new InvalidVersionException(this.Version);
            }
            else
            {
                return null;
            }
        }
    }
}
