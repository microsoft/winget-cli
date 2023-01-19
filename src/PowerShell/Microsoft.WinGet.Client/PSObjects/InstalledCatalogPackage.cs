// -----------------------------------------------------------------------------
// <copyright file="InstalledCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System.Linq;

    /// <summary>
    /// CatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public class InstalledCatalogPackage : CatalogPackage
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstalledCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        public InstalledCatalogPackage(Management.Deployment.CatalogPackage catalogPackage)
        {
            this.Id = catalogPackage.Id;
            this.Name = catalogPackage.Name;
            this.IsUpdateAvailable = catalogPackage.IsUpdateAvailable;
            this.Version = catalogPackage.InstalledVersion.Version;
            this.Source = catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;

            if (this.IsUpdateAvailable)
            {
                this.AvailableVersions = catalogPackage.AvailableVersions.Select(i => i.Version).ToArray();
            }
        }
    }
}
