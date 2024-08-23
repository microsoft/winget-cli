// -----------------------------------------------------------------------------
// <copyright file="PackageManagerWrapper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Windows.Foundation;

    /// <summary>
    /// Wrapper for PackageManager that handles rpc disconnections.
    /// The object is disconnected when the server is killed or on an update.
    /// </summary>
    internal sealed class PackageManagerWrapper
    {
        private static readonly Lazy<PackageManagerWrapper> Lazy = new (() => new PackageManagerWrapper());

        private PackageManager packageManager = null!;

        private PackageManagerWrapper()
        {
        }

        /// <summary>
        /// Gets the instance object.
        /// </summary>
        public static PackageManagerWrapper Instance
        {
            get { return Lazy.Value; }
        }

        /// <summary>
        /// Wrapper for InstallPackageAsync.
        /// </summary>
        /// <param name="package">The package to install.</param>
        /// <param name="options">The install options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<InstallResult, InstallProgress> InstallPackageAsync(CatalogPackage package, InstallOptions options)
        {
            return this.Execute(
                () => this.packageManager.InstallPackageAsync(package, options),
                false);
        }

        /// <summary>
        /// Wrapper for UpgradePackageAsync.
        /// </summary>
        /// <param name="package">The package to upgrade.</param>
        /// <param name="options">The install options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<InstallResult, InstallProgress> UpgradePackageAsync(CatalogPackage package, InstallOptions options)
        {
            return this.Execute(
                () => this.packageManager.UpgradePackageAsync(package, options),
                false);
        }

        /// <summary>
        /// Wrapper for UninstallPackageAsync.
        /// </summary>
        /// <param name="package">The package to uninstall.</param>
        /// <param name="options">The uninstall options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<UninstallResult, UninstallProgress> UninstallPackageAsync(CatalogPackage package, UninstallOptions options)
        {
            return this.Execute(
                () => this.packageManager.UninstallPackageAsync(package, options),
                false);
        }

        /// <summary>
        /// Wrapper for DownloadPackageAsync.
        /// </summary>
        /// <param name="package">The package to download.</param>
        /// <param name="options">The download options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> DownloadPackageAsync(CatalogPackage package, DownloadOptions options)
        {
            return this.Execute(
                () => this.packageManager.DownloadPackageAsync(package, options),
                false);
        }

        /// <summary>
        /// Wrapper for RepairPackagesAsync.
        /// </summary>
        /// <param name="package">The package to repair.</param>
        /// <param name="options">The repair options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<RepairResult, RepairProgress> RepairPackageAsync(CatalogPackage package, RepairOptions options)
        {
            return this.Execute(
                () => this.packageManager.RepairPackageAsync(package, options),
                false);
        }

        /// <summary>
        /// Wrapper for GetPackageCatalogs.
        /// </summary>
        /// <returns>A list of PackageCatalogReferences.</returns>
        public IReadOnlyList<PackageCatalogReference> GetPackageCatalogs()
        {
            return this.Execute(
                () => this.packageManager.GetPackageCatalogs(),
                true);
        }

        /// <summary>
        /// Wrapper for GetPackageCatalogByName.
        /// </summary>
        /// <param name="source">The name of the source.</param>
        /// <returns>A PackageCatalogReference.</returns>
        public PackageCatalogReference GetPackageCatalogByName(string source)
        {
            return this.Execute(
                () => this.packageManager.GetPackageCatalogByName(source),
                true);
        }

        /// <summary>
        /// Wrapper for CreateCompositePackageCatalog.
        /// </summary>
        /// <param name="options">CreateCompositePackageCatalogOptions.</param>
        /// <returns>A PackageCatalogReference.</returns>
        public PackageCatalogReference CreateCompositePackageCatalog(CreateCompositePackageCatalogOptions options)
        {
            return this.Execute(
                () => this.packageManager.CreateCompositePackageCatalog(options),
                false);
        }

        private TReturn Execute<TReturn>(Func<TReturn> func, bool canRetry)
        {
            if (Utilities.UsesInProcWinget && Utilities.ThreadIsSTA)
            {
                // If you failed here, then you didn't wrap your call in ManagementDeploymentCommand.Execute
                throw new SingleThreadedApartmentException();
            }

            bool stopRetry = false;
            while (true)
            {
                if (this.packageManager == null)
                {
                    this.packageManager = ManagementDeploymentFactory.Instance.CreatePackageManager();
                }

                try
                {
                    return func();
                }
                catch (COMException ex) when (ex.HResult == ErrorCode.RpcServerUnavailable || ex.HResult == ErrorCode.RpcCallFailed)
                {
                    this.packageManager = null!;

                    if (stopRetry || !canRetry)
                    {
                        throw;
                    }

                    stopRetry = true;
                }
            }
        }
    }
}
