// -----------------------------------------------------------------------
// <copyright file="WinGetManifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Api
{
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Exceptions;
    using Microsoft.WinGetUtil.Interfaces;

    /// <summary>
    /// Wrapper class around WinGetUtil manifest native implementation.
    /// For dll entry points are defined here:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    public sealed class WinGetManifest : IWinGetManifest
    {
        private readonly IntPtr manifestHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetManifest"/> class.
        /// </summary>
        /// <param name="manifestHandle">Handle of the manifest.</param>
        /// <param name="loggingContext">Logging Context.</param>
        internal WinGetManifest(IntPtr manifestHandle)
        {
            this.manifestHandle = manifestHandle;
        }

        /// <inheritdoc/>
        public ManifestValidationResult ValidateManifestV3(
            IntPtr indexHandle,
            WinGetValidateManifestOptionV2 option,
            WinGetValidateManifestOperationType operationType)
        {
            try
            {
                WinGetValidateManifestV3(
                    this.manifestHandle,
                    indexHandle,
                    out WinGetValidateManifestResult result,
                    out string failureOrWarningMessage,
                    option,
                    operationType);

                return new ManifestValidationResult(result == WinGetValidateManifestResult.Success, failureOrWarningMessage, result);
            }
            catch (Exception e)
            {
                throw new WinGetManifestException(e);
            }
        }

        /// <summary>
        /// Dispose method.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose method to free the manifest handle.
        /// </summary>
        /// <param name="disposing">Bool value indicating if Dispose is being run.</param>
        public void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.manifestHandle != IntPtr.Zero)
                {
                    WinGetCloseManifest(this.manifestHandle);
                }
            }
        }

        /// <summary>
        /// Closes the manifest handle.
        /// </summary>
        /// <param name="manifest">Handle of the manifest.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetCloseManifest(IntPtr manifest);

        /// <summary>
        /// Validates a given manifest. Returns an enum for validation result and
        /// a string representing validation errors if validation failed.
        /// </summary>
        /// <param name="manifestHandle">Handle of the manifest.</param>
        /// <param name="indexHandle">Handle of the index.</param>
        /// <param name="result">Out validation result.</param>
        /// <param name="failureMessage">Out string failure message, if any.</param>
        /// <param name="option">Validate manifest option.</param>
        /// <param name="operationType">Validate manifest operation type.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetValidateManifestV3(
            IntPtr manifestHandle,
            IntPtr indexHandle,
            out WinGetValidateManifestResult result,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            WinGetValidateManifestOptionV2 option,
            WinGetValidateManifestOperationType operationType);
    }
}
