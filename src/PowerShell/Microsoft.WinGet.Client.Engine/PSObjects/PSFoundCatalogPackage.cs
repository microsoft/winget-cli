// -----------------------------------------------------------------------------
// <copyright file="PSFoundCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using Microsoft.Management.Deployment;

    /// <summary>
    /// FoundCatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public sealed class PSFoundCatalogPackage : PSCatalogPackage
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSFoundCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSFoundCatalogPackage(CatalogPackage catalogPackage)
            : base(catalogPackage)
        {
        }

        /// <summary>
        /// Gets the default install version of the catalog package.
        /// </summary>
        public string Version
        {
            get { return this.CatalogPackageCOM.DefaultInstallVersion.Version; }
        }
    }
}
