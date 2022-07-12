// -----------------------------------------------------------------------------
// <copyright file="BaseLifecycleCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Errors;
    using Microsoft.WinGet.Client.Helpers;

    /// <summary>
    /// This is the base class for commands which operate on a specific package version.
    /// </summary>
    public class BaseLifecycleCommand : BaseFinderCommand
    {
        private string log;

        /// <summary>
        /// Gets or sets the package to directly install.
        /// </summary>
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
                string prefix = Path.IsPathRooted(value)
                    ? string.Empty
                    : this.SessionState.Path.CurrentFileSystemLocation + @"\";

                this.log = prefix + value;
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
            var package = this.GetCatalogPackage(behavior);
            var version = this.GetPackageVersionId(package);
            var confirm = this.ConfirmOperation(package, version);
            if (confirm)
            {
                callback(package, version);
            }
        }

        private CatalogPackage GetCatalogPackage(CompositeSearchBehavior behavior)
        {
            if (this.ParameterSetName == Constants.GivenSet)
            {
                return this.CatalogPackage;
            }
            else
            {
                var results = this.FindPackages(behavior, 0);
                if (results.Count == 1)
                {
                    return results[0].CatalogPackage;
                }
                else if (results.Count == 0)
                {
                    throw new RuntimeException(Constants.ResourceManager.GetString("ExceptionMessages_NoPackagesFound"));
                }
                else
                {
                    throw new VagueCriteriaException(results);
                }
            }
        }

        private PackageVersionId GetPackageVersionId(CatalogPackage package)
        {
            if (this.Version != null)
            {
                var versions = package.AvailableVersions;

                for (var i = 0; i < versions.Count; i++)
                {
                    if (versions[i].Version.CompareTo(this.Version) == 0)
                    {
                        return versions[i];
                    }
                }

                throw new ArgumentException(Constants.ResourceManager.GetString("ExceptionMessages_VersionNotFound"));
            }
            else
            {
                return null;
            }
        }

        private bool ConfirmOperation(CatalogPackage package, PackageVersionId version)
        {
            string target = package.ToString(version);
            return this.ShouldProcess(target);
        }
    }
}
