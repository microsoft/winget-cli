// -----------------------------------------------------------------------------
// <copyright file="CatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System.Linq;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Exceptions;

    /// <summary>
    /// CatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public class CatalogPackage
    {
        private Management.Deployment.CatalogPackage catalogPackage;

        /// <summary>
        /// Initializes a new instance of the <see cref="CatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">CatalogPackage COM object.</param>
        public CatalogPackage(Management.Deployment.CatalogPackage catalogPackage)
        {
            this.catalogPackage = catalogPackage;
        }

        /// <summary>
        /// Gets the name of the catalog package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.catalogPackage.Name;
            }
        }

        /// <summary>
        /// Gets the id of the catalog package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.catalogPackage.Id;
            }
        }

        /// <summary>
        /// Gets or sets the version of the catalog package.
        /// </summary>
        public string Version { get; protected set; }

        /// <summary>
        /// Gets a value indicating whether an update is available.
        /// </summary>
        public bool IsUpdateAvailable
        {
            get
            {
                return this.catalogPackage.IsUpdateAvailable;
            }
        }

        /// <summary>
        /// Gets the source name of the catalog package.
        /// </summary>
        public string Source
        {
            get
            {
                return this.catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;
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
        /// Gets a list of available package version ids for the package.
        /// </summary>
        private PackageVersionId[] AvailablePackageVersionIds
        {
            get
            {
                return this.catalogPackage.AvailableVersions.ToArray();
            }
        }

        /// <summary>
        /// Checks the installed status of the catalog package.
        /// </summary>
        /// <returns>CheckInstalledStatus string.</returns>
        public string CheckInstalledStatus()
        {
            return this.catalogPackage.CheckInstalledStatus().Status.ToString();
        }

        /// <summary>
        /// Gets the PackageVersionInfo PSObject that corresponds with the version string.
        /// </summary>
        /// <param name="version">Version string.</param>
        /// <returns>PackageVersionInfo PSObject.</returns>
        /// <exception cref="NoPackageFoundException">Throws an exception if no package is found.</exception>
        public PackageVersionInfo GetPackageVersionInfo(string version)
        {
            // get specific version that matches
            PackageVersionId packageVersionId = this.AvailablePackageVersionIds.FirstOrDefault(x => x.Version == version);
            if (packageVersionId != null)
            {
                return new PackageVersionInfo(this.catalogPackage.GetPackageVersionInfo(packageVersionId));
            }
            else
            {
                throw new NoPackageFoundException();
            }
        }
    }
}
