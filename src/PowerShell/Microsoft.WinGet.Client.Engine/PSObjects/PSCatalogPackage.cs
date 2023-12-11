// -----------------------------------------------------------------------------
// <copyright file="PSCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System.Linq;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Exceptions;

    /// <summary>
    /// CatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public abstract class PSCatalogPackage
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">CatalogPackage COM object.</param>
        internal PSCatalogPackage(Management.Deployment.CatalogPackage catalogPackage)
        {
            this.CatalogPackageCOM = catalogPackage;
        }

        /// <summary>
        /// Gets the name of the catalog package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.CatalogPackageCOM.Name;
            }
        }

        /// <summary>
        /// Gets the id of the catalog package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.CatalogPackageCOM.Id;
            }
        }

        /// <summary>
        /// Gets a value indicating whether an update is available.
        /// </summary>
        public bool IsUpdateAvailable
        {
            get
            {
                return this.CatalogPackageCOM.IsUpdateAvailable;
            }
        }

        /// <summary>
        /// Gets the source name of the catalog package.
        /// </summary>
        public string? Source
        {
            get
            {
                return this.CatalogPackageCOM.DefaultInstallVersion?.PackageCatalog.Info.Name;
            }
        }

        /// <summary>
        /// Gets list of strings representing the available versions.
        /// </summary>
        public string[] AvailableVersions
        {
            get
            {
                return this.AvailablePackageVersionIds.Select(i => i.Version).ToArray();
            }
        }

        /// <summary>
        /// Gets the catalog package COM object.
        /// </summary>
        internal CatalogPackage CatalogPackageCOM { get; private set; }

        /// <summary>
        /// Gets a list of available package version ids for the package.
        /// </summary>
        private PackageVersionId[] AvailablePackageVersionIds
        {
            get
            {
                return this.CatalogPackageCOM.AvailableVersions.ToArray();
            }
        }

        /// <summary>
        /// Checks the installed status of the catalog package.
        /// </summary>
        /// <returns>CheckInstalledStatus string.</returns>
        public string CheckInstalledStatus()
        {
            return this.CatalogPackageCOM.CheckInstalledStatus().Status.ToString();
        }

        /// <summary>
        /// Gets the PackageVersionInfo PSObject that corresponds with the version string.
        /// </summary>
        /// <param name="version">Version string.</param>
        /// <returns>PackageVersionInfo PSObject.</returns>
        /// <exception cref="NoPackageFoundException">Throws an exception if no package is found.</exception>
        public PSPackageVersionInfo GetPackageVersionInfo(string version)
        {
            // get specific version that matches
            PackageVersionId? packageVersionId = this.AvailablePackageVersionIds.FirstOrDefault(x => x.Version == version);
            if (packageVersionId != null)
            {
                return new PSPackageVersionInfo(this.CatalogPackageCOM.GetPackageVersionInfo(packageVersionId));
            }
            else
            {
                throw new NoPackageFoundException();
            }
        }
    }
}
