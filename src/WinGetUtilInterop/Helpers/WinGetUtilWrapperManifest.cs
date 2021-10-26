// -----------------------------------------------------------------------
// <copyright file="WinGetUtilWrapperManifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Helpers
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Wrapper class around WinGetUtil manifest native implementation.
    /// For dll entry points are defined here:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    public sealed class WinGetUtilWrapperManifest
    {
        /// <summary>
        /// The error from the installer.
        /// Must be in sync with WinGetValidateManifestOption at:
        ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
        /// </summary>
        public enum ValidateManifestOption
        {
            /// <summary>
            /// Default validation behavior.
            /// </summary>
            Default,

            /// <summary>
            /// Schema validation only.
            /// </summary>
            SchemaValidationOnly,
        }

        /// <summary>
        /// Validates the manifest is compliant.
        /// </summary>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="mergedManifestPath">Merged manifest output path.</param>
        /// <param name="option">Desired validate manifest option.</param>
        /// <returns>Message from manifest validation.</returns>
        public static (bool isValid, string message) ValidateManifest(
            string manifestPath,
            string mergedManifestPath = null,
            ValidateManifestOption option = ValidateManifestOption.Default)
        {
            WinGetValidateManifestV2(
                manifestPath,
                out bool succeeded,
                out string failureOrWarningMessage,
                mergedManifestPath,
                option);

            return (succeeded, failureOrWarningMessage);
        }

        /// <summary>
        /// Validates a given manifest. Returns a bool for validation result and
        /// a string representing validation errors if validation failed.
        /// </summary>
        /// <param name="manifestPath">Path to manifest file.</param>
        /// <param name="succeeded">Out bool is validation succeeded.</param>
        /// <param name="failureMessage">Out string failure message, if any.</param>
        /// <param name="mergedManifestPath">Path to merged manifest file. Empty means no merged manifest needed.</param>
        /// <param name="option">Validate manifest option.</param>
        /// <returns>HRESULT.</returns>
        [DllImport("WinGetUtil.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetValidateManifestV2(
            string manifestPath,
            [MarshalAs(UnmanagedType.U1)] out bool succeeded,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            string mergedManifestPath,
            ValidateManifestOption option);
    }
}
