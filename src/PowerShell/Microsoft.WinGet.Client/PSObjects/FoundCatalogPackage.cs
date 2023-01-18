// -----------------------------------------------------------------------------
// <copyright file="FoundCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System;
    using System.Linq;

    /// <summary>
    /// FoundCatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public class FoundCatalogPackage
    {
        /// <summary>
        /// The id of the catalog package.
        /// </summary>
        public readonly string Id;

        /// <summary>
        /// The name of the catalog package.
        /// </summary>
        public readonly string Name;

        /// <summary>
        /// The version of the catalog package.
        /// </summary>
        public readonly string Version;

        /// <summary>
        /// A boolean value indicating whether an update is available.
        /// </summary>
        public readonly bool IsUpdateAvailable;

        /// <summary>
        /// The source of the catalog package.
        /// </summary>
        public readonly string Source;

        /// <summary>
        /// A list of strings representing the available versions.
        /// </summary>
        public readonly string[] AvailableVersions;

        /// <summary>
        /// Initializes a new instance of the <see cref="FoundCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        public FoundCatalogPackage(Management.Deployment.CatalogPackage catalogPackage)
        {
            this.Id = catalogPackage.Id;
            this.Name = catalogPackage.Name;
            this.IsUpdateAvailable = catalogPackage.IsUpdateAvailable;
            this.Version = catalogPackage.DefaultInstallVersion.Version;
            this.Source = catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;

            //if (this.IsUpdateAvailable)
            //{
            //    this.AvailableVersions = catalogPackage.AvailableVersions.Select(i => i.Version).ToArray();
            //}
        }
    }
}
