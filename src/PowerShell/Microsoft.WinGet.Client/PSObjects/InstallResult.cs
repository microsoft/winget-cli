// -----------------------------------------------------------------------------
// <copyright file="InstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System;

    /// <summary>
    /// InstallResult wrapper object for displaying to PowerShell.
    /// </summary>
    public class InstallResult
    {
        private Management.Deployment.InstallResult installResult;

        /// <summary>
        /// Initializes a new instance of the <see cref="InstallResult"/> class.
        /// </summary>
        /// <param name="installResult">The install result COM object.</param>
        public InstallResult(Management.Deployment.InstallResult installResult)
        {
            this.installResult = installResult;
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
    }
}
