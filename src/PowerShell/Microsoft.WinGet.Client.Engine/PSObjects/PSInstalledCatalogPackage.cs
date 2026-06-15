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
        private readonly Func<CatalogPackage, bool> isPinnedLookup;
        private bool? isPinned;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSInstalledCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        /// <param name="isPinnedLookup">
        /// Optional lookup used to determine whether the package is pinned. When not supplied,
        /// the lookup is resolved on demand through the package manager.
        /// </param>
        internal PSInstalledCatalogPackage(CatalogPackage catalogPackage, Func<CatalogPackage, bool>? isPinnedLookup = null)
            : base(catalogPackage)
        {
            this.isPinnedLookup = isPinnedLookup ?? this.DefaultIsPinnedLookup;
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
                    this.isPinned = this.isPinnedLookup(this.CatalogPackageCOM);
                }

                return this.isPinned.Value;
            }
        }

        private bool DefaultIsPinnedLookup(CatalogPackage catalogPackage)
        {
            return PackageManagerWrapper.Instance.GetPins(catalogPackage).Count > 0;
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
