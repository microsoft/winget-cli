// -----------------------------------------------------------------------------
// <copyright file="FoundCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
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
            : base(catalogPackage)
        {
        }

        /// <inheritdoc/>
        public override string Version
        {
            get { return this.CatalogPackageCOM.DefaultInstallVersion.Version; }
        }
    }
}
