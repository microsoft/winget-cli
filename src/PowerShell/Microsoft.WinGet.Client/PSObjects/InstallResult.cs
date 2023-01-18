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
        /// The correlation data of the install result.
        /// </summary>
        public readonly string CorrelationData;

        /// <summary>
        /// The extended error code exception of the failed install result.
        /// </summary>
        public readonly Exception ExtendedErrorCode;

        /// <summary>
        /// A boolean value indicating whether a reboot is required.
        /// </summary>
        public readonly bool RebootRequired;

        /// <summary>
        /// The error code of an install.
        /// </summary>
        public readonly uint InstallerErrorCode;

        /// <summary>
        /// The status of the install.
        /// </summary>
        public readonly InstallResultStatus Status;

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
    }
}
