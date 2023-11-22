// -----------------------------------------------------------------------------
// <copyright file="CatalogPackageExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Extensions
{
    using Microsoft.Management.Deployment;

    /// <summary>
    /// Extensions for the <see cref="CatalogPackage" /> class.
    /// </summary>
    internal static class CatalogPackageExtensions
    {
        /// <summary>
        /// Converts a <see cref="CatalogPackage" /> to a string previewing the specified version.
        /// </summary>
        /// <param name="package">A <see cref="CatalogPackage" /> instance.</param>
        /// <param name="version">A <see cref="PackageVersionId" /> instance. If null, the latest available version is used.</param>
        /// <returns>A <see cref="string" /> instance.</returns>
        public static string ToString(
            this CatalogPackage package,
            PackageVersionId? version)
        {
            if ((version != null) || (package.AvailableVersions.Count > 0))
            {
                string versionString = (version is null)
                    ? package.AvailableVersions[0].Version
                    : version.Version;
                return $"{package.Name} [{package.Id}] Version {versionString}";
            }
            else
            {
                // There were no available versions!
                return $"{package.Name} [{package.Id}]";
            }
        }
    }
}
