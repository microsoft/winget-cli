// -----------------------------------------------------------------------------
// <copyright file="PSUninstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// UninstallResult wrapper object for displaying to PowerShell.
    /// </summary>
    public sealed class PSUninstallResult
    {
        private readonly UninstallResult uninstallResult;
        private readonly CatalogPackage catalogPackage;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSUninstallResult"/> class.
        /// </summary>
        /// <param name="uninstallResult">The uninstall result COM object.</param>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSUninstallResult(UninstallResult uninstallResult, CatalogPackage catalogPackage)
        {
            this.uninstallResult = uninstallResult;
            this.catalogPackage = catalogPackage;
        }

        /// <summary>
        /// Gets the id of the uninstalled package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.catalogPackage.Id;
            }
        }

        /// <summary>
        /// Gets the name of the uninstalled package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.catalogPackage.Name;
            }
        }

        /// <summary>
        /// Gets the source name of the uninstalled package.
        /// </summary>
        public string Source
        {
            get
            {
                return this.catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;
            }
        }

        /// <summary>
        /// Gets the correlation data of the uninstall result.
        /// </summary>
        public string CorrelationData
        {
            get
            {
                return this.uninstallResult.CorrelationData;
            }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed uninstall result.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get
            {
                return this.uninstallResult.ExtendedErrorCode;
            }
        }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired
        {
            get
            {
                return this.uninstallResult.RebootRequired;
            }
        }

        /// <summary>
        /// Gets the status of the uninstall.
        /// </summary>
        public string Status
        {
            get
            {
                return this.uninstallResult.Status.ToString();
            }
        }

        /// <summary>
        /// Gets the error code of an uninstall.
        /// </summary>
        public uint UninstallerErrorCode
        {
            get
            {
                return this.uninstallResult.UninstallerErrorCode;
            }
        }

        /// <summary>
        /// If the uninstall succeeded.
        /// </summary>
        /// <returns>True if uninstall succeeded.</returns>
        public bool Succeeded()
        {
            return this.uninstallResult.Status == UninstallResultStatus.Ok;
        }

        /// <summary>
        /// Message with error information.
        /// </summary>
        /// <returns>Error message.</returns>
        public string ErrorMessage()
        {
            return $"UninstallStatus '{this.Status}' UninstallerErrorCode '{this.UninstallerErrorCode}' ExtendedError '{this.ExtendedErrorCode.HResult}'";
        }
    }
}
