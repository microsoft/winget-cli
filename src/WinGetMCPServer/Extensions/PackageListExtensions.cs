// -----------------------------------------------------------------------------
// <copyright file="PackageListExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Extensions
{
    using Microsoft.Management.Deployment;
    using WinGetMCPServer.Response;

    /// <summary>
    /// Extensions for List<FindPackageResult>.
    /// </summary>
    internal static class PackageListExtensions
    {
        /// <summary>
        /// Adds the packages from a find to the list.
        /// </summary>
        /// <param name="list">The list to add to.</param>
        /// <param name="findResult">A find result.</param>
        /// <returns>The list.</returns>
        static public List<FindPackageResult> AddPackages(this List<FindPackageResult> list, FindPackagesResult findResult)
        {
            for (int i = 0; i < findResult.Matches!.Count; ++i)
            {
                list.Add(FindPackageResultFromCatalogPackage(findResult.Matches[i].CatalogPackage));
            }

            return list;
        }

        /// <summary>
        /// Creates a <see cref="FindPackageResult"/> from the specified <see cref="CatalogPackage"/>.
        /// </summary>
        /// <remarks>The method populates the <see cref="FindPackageResult"/> with details such as the
        /// package identifier, name, installation status, and update availability. If the package is installed,
        /// additional information such as the catalog name and installed location is included.</remarks>
        /// <param name="package">The catalog package from which to create the result. Cannot be null.</param>
        /// <returns>A <see cref="FindPackageResult"/> representing the state of the specified catalog package, including
        /// installation status and available updates.</returns>
        static public FindPackageResult FindPackageResultFromCatalogPackage(CatalogPackage package)
        {
            FindPackageResult findPackageResult = new FindPackageResult()
            {
                Identifier = package.Id,
                Name = package.Name,
            };

            var installedVersion = package.InstalledVersion;
            findPackageResult.IsInstalled = installedVersion != null;

            if (installedVersion != null)
            {
                findPackageResult.Source = installedVersion.PackageCatalog?.Info?.Name;
                if (string.IsNullOrEmpty(findPackageResult.Source))
                {
                    findPackageResult.Source = package.DefaultInstallVersion?.PackageCatalog?.Info?.Name;
                }

                findPackageResult.InstalledVersion = installedVersion.Version;
                findPackageResult.IsUpdateAvailable = package.IsUpdateAvailable;

                string installLocation = installedVersion.GetMetadata(PackageVersionMetadataField.InstalledLocation);
                if (!string.IsNullOrEmpty(installLocation))
                {
                    findPackageResult.InstalledLocation = installLocation;
                }
            }
            else
            {
                findPackageResult.Source = package.DefaultInstallVersion.PackageCatalog.Info.Name;
            }

            return findPackageResult;
        }
    }
}
