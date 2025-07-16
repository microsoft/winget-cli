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
        static public List<FindPackageResult> AddPackages(this List<FindPackageResult> list, FindPackagesResult findResult)
        {
            for (int i = 0; i < findResult.Matches!.Count; ++i)
            {
                list.Add(FindPackageResultFromCatalogPackage(findResult.Matches[i].CatalogPackage));
            }

            return list;
        }
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
                findPackageResult.Catalog = installedVersion.PackageCatalog?.Info?.Name;
                if (string.IsNullOrEmpty(findPackageResult.Catalog))
                {
                    findPackageResult.Catalog = package.DefaultInstallVersion?.PackageCatalog?.Info?.Name;
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
                findPackageResult.Catalog = package.DefaultInstallVersion.PackageCatalog.Info.Name;
            }

            return findPackageResult;
        }
    }
}
