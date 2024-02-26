// -----------------------------------------------------------------------------
// <copyright file="PSDownloadResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PSDownloadResult.
    /// </summary>
    public sealed class PSDownloadResult
    {
        private readonly DownloadResult downloadResult;
        private readonly CatalogPackage catalogPackage;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSDownloadResult"/> class.
        /// </summary>
        /// <param name="downloadResult">The download result COM object.</param>
        /// <param name="catalogPackage">The catalog package COM object.</param>
        internal PSDownloadResult(DownloadResult downloadResult, CatalogPackage catalogPackage)
        {
            this.downloadResult = downloadResult;
            this.catalogPackage = catalogPackage;
        }

        /// <summary>
        /// Gets the id of the downloaded package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.catalogPackage.Id;
            }
        }

        /// <summary>
        /// Gets the name of the downloaded package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.catalogPackage.Name;
            }
        }

        /// <summary>
        /// Gets the source name of the downloaded package.
        /// </summary>
        public string Source
        {
            get
            {
                return this.catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;
            }
        }

        /// <summary>
        /// Gets the correlation data of the downloaded result.
        /// </summary>
        public string CorrelationData
        {
            get
            {
                return this.downloadResult.CorrelationData;
            }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed download result.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get
            {
                return this.downloadResult.ExtendedErrorCode;
            }
        }

        /// <summary>
        /// Gets the status of the download.
        /// </summary>
        public string Status
        {
            get
            {
                return this.downloadResult.Status.ToString();
            }
        }

        /// <summary>
        /// If the download succeeded.
        /// </summary>
        /// <returns>True if installation succeeded.</returns>
        public bool Succeeded()
        {
            return this.downloadResult.Status == DownloadResultStatus.Ok;
        }

        /// <summary>
        /// Message with error information.
        /// </summary>
        /// <returns>Error message.</returns>
        public string ErrorMessage()
        {
            return $"DownloadStatus '{this.Status}' ExtendedError '{this.ExtendedErrorCode.HResult}'";
        }
    }
}
