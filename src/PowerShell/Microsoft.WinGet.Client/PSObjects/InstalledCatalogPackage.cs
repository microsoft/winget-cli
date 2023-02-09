// -----------------------------------------------------------------------------
// <copyright file="InstalledCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
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
            : base(catalogPackage)
        {
        }

        /// <inheritdoc/>
        public override string Version
        {
            get { return this.CatalogPackageCOM.InstalledVersion.Version; }
        }
    }
}
