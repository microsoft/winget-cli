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
        private readonly string correlationData;
        private readonly Exception extendedErrorCode;
        private readonly bool rebootRequired;
        private readonly uint uninstallerErrorCode;
        private readonly UninstallResultStatus status;

        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallResult"/> class.
        /// </summary>
        /// <param name="uninstallResult">The uninstall result COM object.</param>
        public UninstallResult(Management.Deployment.UninstallResult uninstallResult)
        {
            this.correlationData = uninstallResult.CorrelationData;
            this.extendedErrorCode = uninstallResult.ExtendedErrorCode;
            this.rebootRequired = uninstallResult.RebootRequired;
            this.uninstallerErrorCode = uninstallResult.UninstallerErrorCode;
            this.status = uninstallResult.Status;
        }

        /// <summary>
        /// Gets the correlation data of the uninstall result.
        /// </summary>
        public string CorrelationData
        {
            get { return this.correlationData; }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed uninstall result.
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
        /// Gets the error code of an uninstall.
        /// </summary>
        public uint UninstallerErrorCode
        {
            get { return this.uninstallerErrorCode; }
        }

        /// <summary>
        /// Gets the status of the uninstall.
        /// </summary>
        public UninstallResultStatus Status
        {
            get { return this.status; }
        }
    }
}
