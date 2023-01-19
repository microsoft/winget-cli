// -----------------------------------------------------------------------------
// <copyright file="InstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// InstallResult wrapper object for displaying to PowerShell.
    /// </summary>
    public class InstallResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallResult"/> class.
        /// </summary>
        /// <param name="installResult">The install result COM object.</param>
        public InstallResult(Management.Deployment.InstallResult installResult)
        {
            this.CorrelationData = installResult.CorrelationData;
            this.ExtendedErrorCode = installResult.ExtendedErrorCode;
            this.RebootRequired = installResult.RebootRequired;
            this.InstallerErrorCode = installResult.InstallerErrorCode;
            this.Status = installResult.Status;
        }

        /// <summary>
        /// Gets the correlation data of the install result.
        /// </summary>
        public string CorrelationData { get; private set; }

        /// <summary>
        /// Gets the extended error code exception of the failed install result.
        /// </summary>
        public Exception ExtendedErrorCode { get; private set; }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired { get; private set; }

        /// <summary>
        /// Gets the error code of an install.
        /// </summary>
        public uint InstallerErrorCode { get; private set; }

        /// <summary>
        /// Gets the status of the install.
        /// </summary>
        public InstallResultStatus Status { get; private set; }
    }
}
