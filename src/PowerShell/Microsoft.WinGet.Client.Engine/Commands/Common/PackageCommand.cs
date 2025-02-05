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
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// This is the base class for commands which operate on a specific package and version i.e.,
    /// the "install", "uninstall", "download", and "upgrade" commands.
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
        protected PSCatalogPackage? CatalogPackage { get; set; } = null;

        /// <summary>
        /// Gets or sets the version to install.
        /// </summary>
        protected string? Version { get; set; }

        /// <summary>
        /// Executes a command targeting a specific package version.
        /// </summary>
        /// <typeparam name="TResult">Type of callback's result.</typeparam>
        /// <param name="behavior">The <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="match">The match option.</param>
        /// <param name="callback">The method to call after retrieving the package and version to operate upon.</param>
        /// <returns>Result of the callback.</returns>
        internal async Task<Tuple<TResult, CatalogPackage>?> GetPackageAndExecuteAsync<TResult>(
            CompositeSearchBehavior behavior,
            PackageFieldMatchOption match,
            Func<CatalogPackage, PackageVersionId?, Task<TResult>> callback)
            where TResult : class
        {
            CatalogPackage package = this.GetCatalogPackage(behavior, match);
            PackageVersionId? version = this.GetPackageVersionId(package);
            if (this.ShouldProcess(package.ToString(version)))
            {
                var result = await callback(package, version);
                return new Tuple<TResult, CatalogPackage>(result, package);
            }

            return null;
        }

        /// <summary>
        /// Sets the find package options for a query input that is looking for a specific package.
        /// DO NOT pass PackageFieldMatchOption WinRT enum type in this method.
        /// That will cause the type to attempt to be loaded in the construction
        /// of this method and throw a different exception for Windows PowerShell.
        /// </summary>
        /// <param name="options">The options object.</param>
        /// <param name="match">The match type.</param>
        /// <param name="value">The query value.</param>
        internal override void SetQueryInFindPackagesOptions(
            ref FindPackagesOptions options,
            string match,
            string? value)
        {
            var matchOption = PSEnumHelpers.ToPackageFieldMatchOption(match);
            foreach (PackageMatchField field in new PackageMatchField[] { PackageMatchField.Id, PackageMatchField.Name, PackageMatchField.Moniker })
            {
                var selector = ManagementDeploymentFactory.Instance.CreatePackageMatchFilter();
                selector.Field = field;
                selector.Value = value ?? string.Empty;
                selector.Option = matchOption;
                options.Selectors.Add(selector);
            }
        }

        private CatalogPackage GetCatalogPackage(CompositeSearchBehavior behavior, PackageFieldMatchOption match)
        {
            if (this.CatalogPackage != null)
            {
                // The package was already provided via a parameter or the pipeline.
                return this.CatalogPackage.CatalogPackageCOM;
            }
            else
            {
                IReadOnlyList<MatchResult> results = this.FindPackages(behavior, 0, match);
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

        private PackageVersionId? GetPackageVersionId(CatalogPackage package)
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
