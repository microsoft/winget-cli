// -----------------------------------------------------------------------------
// <copyright file="PSInstalledCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Helpers;

    /// <summary>
    /// InstalledCatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public sealed class PSInstalledCatalogPackage : PSCatalogPackage
    {
        private bool? isPinned;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSInstalledCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSInstalledCatalogPackage(CatalogPackage catalogPackage)
            : base(catalogPackage)
        {
        }

        /// <summary>
        /// Gets the installed version of the catalog package.
        /// </summary>
        public string InstalledVersion
        {
            get { return this.CatalogPackageCOM.InstalledVersion.Version; }
        }

        /// <summary>
        /// Gets a value indicating whether the package is pinned.
        /// </summary>
        public bool IsPinned
        {
            get
            {
                if (!this.isPinned.HasValue)
                {
                    this.isPinned = PackageManagerWrapper.Instance.GetPins(this.CatalogPackageCOM).Count > 0;
                }

                return this.isPinned.Value;
            }
        }

        /// <summary>
        /// Compares versions.
        /// </summary>
        /// <param name="version">Version.</param>
        /// <returns>PSCompareResult.</returns>
        public PSCompareResult CompareToVersion(string version)
        {
            return this.CatalogPackageCOM.InstalledVersion.CompareToVersion(version) switch
            {
                CompareResult.Unknown => PSCompareResult.Unknown,
                CompareResult.Lesser => PSCompareResult.Lesser,
                CompareResult.Equal => PSCompareResult.Equal,
                CompareResult.Greater => PSCompareResult.Greater,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
