// -----------------------------------------------------------------------------
// <copyright file="PSInstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PSInstallResult.
    /// </summary>
    public sealed class PSInstallResult
    {
        private readonly InstallResult installResult;
        private readonly CatalogPackage catalogPackage;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSInstallResult"/> class.
        /// </summary>
        /// <param name="installResult">The install result COM object.</param>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSInstallResult(InstallResult installResult, CatalogPackage catalogPackage)
        {
            this.installResult = installResult;
            this.catalogPackage = catalogPackage;
        }

        /// <summary>
        /// Gets the id of the installed package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.catalogPackage.Id;
            }
        }

        /// <summary>
        /// Gets the name of the installed package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.catalogPackage.Name;
            }
        }

        /// <summary>
        /// Gets the source name of the installed package.
        /// </summary>
        public string Source
        {
            get
            {
                return this.catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;
            }
        }

        /// <summary>
        /// Gets the correlation data of the install result.
        /// </summary>
        public string CorrelationData
        {
            get
            {
                return this.installResult.CorrelationData;
            }
        }

        /// <summary>
        /// Gets the error code of an install.
        /// </summary>
        public uint InstallerErrorCode
        {
            get
            {
                return this.installResult.InstallerErrorCode;
            }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed install result.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get
            {
                return this.installResult.ExtendedErrorCode;
            }
        }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired
        {
            get
            {
                return this.installResult.RebootRequired;
            }
        }

        /// <summary>
        /// Gets the status of the install.
        /// </summary>
        public string Status
        {
            get
            {
                return this.installResult.Status.ToString();
            }
        }

        /// <summary>
        /// If the installation succeeded.
        /// </summary>
        /// <returns>True if installation succeeded.</returns>
        public bool Succeeded()
        {
            return this.installResult.Status == InstallResultStatus.Ok;
        }

        /// <summary>
        /// Message with error information.
        /// </summary>
        /// <returns>Error message.</returns>
        public string ErrorMessage()
        {
            return $"InstallStatus '{this.Status}' InstallerErrorCode '{this.InstallerErrorCode}' ExtendedError '{this.ExtendedErrorCode.HResult}'";
        }
    }
}
