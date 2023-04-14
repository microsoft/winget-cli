// -----------------------------------------------------------------------------
// <copyright file="PSInstalledCatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using Microsoft.Management.Deployment;
    using System;

    /// <summary>
    /// InstalledCatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public sealed class PSInstalledCatalogPackage : PSCatalogPackage
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSInstalledCatalogPackage"/> class.
        /// </summary>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSInstalledCatalogPackage(CatalogPackage catalogPackage)
            : base(catalogPackage)
        {
        }

        /// <inheritdoc/>
        public override string Version
        {
            get { return this.CatalogPackageCOM.InstalledVersion.Version; }
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
