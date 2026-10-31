// -----------------------------------------------------------------------------
// <copyright file="PSPinResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PSPinResult wraps a PinPackageResult COM object for PowerShell output.
    /// </summary>
    public sealed class PSPinResult
    {
        private readonly PinPackageResult pinResult;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSPinResult"/> class.
        /// </summary>
        /// <param name="pinResult">The PinPackageResult COM object.</param>
        internal PSPinResult(PinPackageResult pinResult)
        {
            this.pinResult = pinResult;
        }

        /// <summary>
        /// Gets the status of the pin operation.
        /// </summary>
        public string Status
        {
            get { return this.pinResult.Status.ToString(); }
        }

        /// <summary>
        /// Gets the extended error code of the pin operation.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get { return this.pinResult.ExtendedErrorCode; }
        }

        /// <summary>
        /// Returns whether the pin operation succeeded.
        /// </summary>
        /// <returns>True if the pin operation succeeded.</returns>
        public bool Succeeded()
        {
            return this.pinResult.Status == PinResultStatus.Ok;
        }

        /// <summary>
        /// Returns a formatted error message.
        /// </summary>
        /// <returns>Error message string.</returns>
        public string ErrorMessage()
        {
            return $"PinStatus: '{this.Status}' ExtendedError: '0x{this.ExtendedErrorCode.HResult:X8}'";
        }
    }
}
