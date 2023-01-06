// -----------------------------------------------------------------------------
// <copyright file="BasePackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Exceptions;

    /// <summary>
    /// This is the base class for commands which operate on a specific package and version i.e.,
    /// the "install", "uninstall", and "upgrade" commands.
    /// </summary>
    public abstract class BasePackageCommand : BaseFinderCommand
    {
        private string log;

        /// <summary>
        /// Gets or sets the package to directly install.
        /// </summary>
        /// <remarks>
        /// Must match the name of the <see cref="CatalogPackage" /> field on the <see cref="MatchResult" /> class.
        /// </remarks>
        [Alias("InputObject")]
        [ValidateNotNull]
        [Parameter(
            ParameterSetName = Constants.GivenSet,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public CatalogPackage CatalogPackage { get; set; }

        /// <summary>
        /// Gets or sets the version to install.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets the path to the logging file.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Log
        {
            get => this.log;
            set
            {
                this.log = Path.IsPathRooted(value)
                    ? value
                    : this.SessionState.Path.CurrentFileSystemLocation + @"\" + value;
            }
        }

        /// <inheritdoc />
        protected override PackageFieldMatchOption GetExactAsMatchOption()
        {
            return this.Exact.ToBool()
                ? PackageFieldMatchOption.Equals
                : PackageFieldMatchOption.EqualsCaseInsensitive;
        }

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
            if (this.ShouldProcess(package.ToString(version)))
            {
                callback(package, version);
            }
        }

        private CatalogPackage GetCatalogPackage(CompositeSearchBehavior behavior)
        {
            if (this.ParameterSetName == Constants.GivenSet)
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
