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
        private readonly string correlationData;
        private readonly Exception extendedErrorCode;
        private readonly bool rebootRequired;
        private readonly uint installerErrorCode;
        private readonly InstallResultStatus status;

        /// <summary>
        /// Initializes a new instance of the <see cref="InstallResult"/> class.
        /// </summary>
        /// <param name="installResult">The install result COM object.</param>
        public InstallResult(Management.Deployment.InstallResult installResult)
        {
            this.correlationData = installResult.CorrelationData;
            this.extendedErrorCode = installResult.ExtendedErrorCode;
            this.rebootRequired = installResult.RebootRequired;
            this.installerErrorCode = installResult.InstallerErrorCode;
            this.status = installResult.Status;
        }

        /// <summary>
        /// Gets the correlation data of the install result.
        /// </summary>
        public string CorrelationData
        {
            get { return this.correlationData; }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed install result.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get { return this.extendedErrorCode; }
        }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired
        {
            get { return this.rebootRequired; }
        }

        /// <summary>
        /// Gets the error code of an install.
        /// </summary>
        public uint InstallerErrorCode
        {
            get { return this.installerErrorCode; }
        }

        /// <summary>
        /// Gets the status of the install.
        /// </summary>
        public InstallResultStatus Status
        {
            get { return this.status; }
        }
    }
}
