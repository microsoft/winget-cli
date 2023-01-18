// -----------------------------------------------------------------------------
// <copyright file="UninstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// UninstallResult wrapper object for displaying to PowerShell.
    /// </summary>
    public class UninstallResult
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
        /// The error code of the uninstall.
        /// </summary>
        public readonly uint UninstallerErrorCode;

        /// <summary>
        /// The status of the uninstall.
        /// </summary>
        public readonly UninstallResultStatus Status;

        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallResult"/> class.
        /// </summary>
        /// <param name="uninstallResult">The uninstalll result COM object.</param>
        public UninstallResult(Management.Deployment.UninstallResult uninstallResult)
        {
            this.CorrelationData = uninstallResult.CorrelationData;
            this.ExtendedErrorCode = uninstallResult.ExtendedErrorCode;
            this.RebootRequired = uninstallResult.RebootRequired;
            this.UninstallerErrorCode = uninstallResult.UninstallerErrorCode;
            this.Status = uninstallResult.Status;
        }
    }
}
