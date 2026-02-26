// -----------------------------------------------------------------------------
// <copyright file="IWinGetManifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    using System;
    using Microsoft.WinGetUtil.Common;

    /// <summary>
    /// Interface for Wrapper class around WinGetUtil index modifier native implementation.
    /// </summary>
    public interface IWinGetManifest : IDisposable
    {
        /// <summary>
        /// Validate the manifest.
        /// </summary>
        /// <param name="indexHandle">The index handle.</param>
        /// <param name="option">Manifest validation option.</param>
        /// <param name="operationType">Manifest validation operation type.</param>
        /// <returns>Result and message from manifest validation.</returns>
        ManifestValidationResult ValidateManifestV3(
            IntPtr indexHandle,
            WinGetValidateManifestOptionV2 option,
            WinGetValidateManifestOperationType operationType);
    }
}
