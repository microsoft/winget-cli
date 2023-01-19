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
        /// Initializes a new instance of the <see cref="UninstallResult"/> class.
        /// </summary>
        /// <param name="uninstallResult">The uninstall result COM object.</param>
        public UninstallResult(Management.Deployment.UninstallResult uninstallResult)
        {
            this.CorrelationData = uninstallResult.CorrelationData;
            this.ExtendedErrorCode = uninstallResult.ExtendedErrorCode;
            this.RebootRequired = uninstallResult.RebootRequired;
            this.UninstallerErrorCode = uninstallResult.UninstallerErrorCode;
            this.Status = uninstallResult.Status;
        }

        /// <summary>
        /// Gets the correlation data of the uninstall result.
        /// </summary>
        public string CorrelationData { get; private set; }

        /// <summary>
        /// Gets the extended error code exception of the failed uninstall result.
        /// </summary>
        public Exception ExtendedErrorCode { get; private set; }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired { get; private set; }

        /// <summary>
        /// Gets the error code of an uninstall.
        /// </summary>
        public uint UninstallerErrorCode { get; private set; }

        /// <summary>
        /// Gets the status of the uninstall.
        /// </summary>
        public UninstallResultStatus Status { get; private set; }
    }
}
