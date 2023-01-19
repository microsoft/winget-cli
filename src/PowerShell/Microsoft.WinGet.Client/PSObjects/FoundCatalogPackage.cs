// -----------------------------------------------------------------------------
// <copyright file="FoundCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System.Linq;

    /// <summary>
    /// FoundCatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public class FoundCatalogPackage
    {
        private readonly string id;
        private readonly string name;
        private readonly string version;
        private readonly bool isUpdateAvailable;
        private readonly string source;
        private readonly string[] availableVersions;

        /// <summary>
        /// Initializes a new instance of the <see cref="FoundCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        public FoundCatalogPackage(Management.Deployment.CatalogPackage catalogPackage)
        {
            this.id = catalogPackage.Id;
            this.name = catalogPackage.Name;
            this.isUpdateAvailable = catalogPackage.IsUpdateAvailable;
            this.version = catalogPackage.DefaultInstallVersion.Version;
            this.source = catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;

            if (this.isUpdateAvailable)
            {
                this.availableVersions = catalogPackage.AvailableVersions.Select(i => i.Version).ToArray();
            }
        }

        /// <summary>
        /// Gets the id of the catalog package.
        /// </summary>
        public string Id
        {
            get { return this.id; }
        }

        /// <summary>
        /// Gets the name of the catalog package.
        /// </summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary>
        /// Gets the version of the catalog package.
        /// </summary>
        public string Version
        {
            get { return this.version; }
        }

        /// <summary>
        /// Gets a value indicating whether an update is available.
        /// </summary>
        public bool IsUpdateAvailable
        {
            get { return this.isUpdateAvailable; }
        }

        /// <summary>
        /// Gets the source of the catalog package.
        /// </summary>
        public string Source
        {
            get { return this.source; }
        }

        /// <summary>
        /// Gets a list of strings representing the available versions.
        /// </summary>
        public string[] AvailableVersions
        {
            get { return this.availableVersions; }
        }
    }
}
