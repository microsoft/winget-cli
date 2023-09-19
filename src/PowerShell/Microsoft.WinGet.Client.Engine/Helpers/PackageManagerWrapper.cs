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
    using Windows.Foundation;

    /// <summary>
    /// Wrapper for PackageManager that handles rpc disconnections.
    /// The object is disconnected when the server is killed or on an update.
    /// </summary>
    internal sealed class PackageManagerWrapper
    {
        private static readonly Lazy<PackageManagerWrapper> Lazy = new (() => new PackageManagerWrapper());

        private PackageManager packageManager = null;

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
            return this.ExecuteWithRetry(
                () => this.packageManager.InstallPackageAsync(package, options));
        }

        /// <summary>
        /// Wrapper for UpgradePackageAsync.
        /// </summary>
        /// <param name="package">The package to upgrade.</param>
        /// <param name="options">The install options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<InstallResult, InstallProgress> UpgradePackageAsync(CatalogPackage package, InstallOptions options)
        {
            return this.ExecuteWithRetry(
                () => this.packageManager.UpgradePackageAsync(package, options));
        }

        /// <summary>
        /// Wrapper for UninstallPackageAsync.
        /// </summary>
        /// <param name="package">The package to uninstall.</param>
        /// <param name="options">The uninstall options.</param>
        /// <returns>An async operation with progress.</returns>
        public IAsyncOperationWithProgress<UninstallResult, UninstallProgress> UninstallPackageAsync(CatalogPackage package, UninstallOptions options)
        {
            return this.ExecuteWithRetry(
                () => this.packageManager.UninstallPackageAsync(package, options));
        }

        /// <summary>
        /// Wrapper for GetPackageCatalogs.
        /// </summary>
        /// <returns>A list of PackageCatalogReferences.</returns>
        public IReadOnlyList<PackageCatalogReference> GetPackageCatalogs()
        {
            return this.ExecuteWithRetry(
                () => this.packageManager.GetPackageCatalogs());
        }

        /// <summary>
        /// Wrapper for GetPackageCatalogByName.
        /// </summary>
        /// <param name="source">The name of the source.</param>
        /// <returns>A PackageCatalogReference.</returns>
        public PackageCatalogReference GetPackageCatalogByName(string source)
        {
            return this.ExecuteWithRetry(
                () => this.packageManager.GetPackageCatalogByName(source));
        }

        /// <summary>
        /// Wrapper for CreateCompositePackageCatalog.
        /// </summary>
        /// <param name="options">CreateCompositePackageCatalogOptions.</param>
        /// <returns>A PackageCatalogReference.</returns>
        public PackageCatalogReference CreateCompositePackageCatalog(CreateCompositePackageCatalogOptions options)
        {
            return this.ExecuteWithRetry(
                () => this.packageManager.CreateCompositePackageCatalog(options));
        }

        private TReturn ExecuteWithRetry<TReturn>(Func<TReturn> func)
        {
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
                    this.packageManager = null;

                    if (stopRetry)
                    {
                        throw;
                    }

                    stopRetry = true;
                }
            }
        }
    }
}
