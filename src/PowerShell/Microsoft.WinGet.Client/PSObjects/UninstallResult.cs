// -----------------------------------------------------------------------------
// <copyright file="UninstallResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    using System;

    /// <summary>
    /// UninstallResult wrapper object for displaying to PowerShell.
    /// </summary>
    public class UninstallResult
    {
        private Management.Deployment.UninstallResult uninstallResult;

        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallResult"/> class.
        /// </summary>
        /// <param name="uninstallResult">The uninstall result COM object.</param>
        public UninstallResult(Management.Deployment.UninstallResult uninstallResult)
        {
            this.uninstallResult = uninstallResult;
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
    }
}
