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
    public class FoundCatalogPackage : CatalogPackage
    {
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

            if (this.IsUpdateAvailable)
            {
                this.AvailableVersions = catalogPackage.AvailableVersions.Select(i => i.Version).ToArray();
            }
        }
    }
}
