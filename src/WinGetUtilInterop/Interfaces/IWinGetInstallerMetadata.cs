// -----------------------------------------------------------------------------
// <copyright file="IWinGetInstallerMetadata.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    using System;

    /// <summary>
    /// Interface for installer metadata collection.
    /// </summary>
    public interface IWinGetInstallerMetadata : IDisposable
    {
        /// <summary>
        /// Completes the installer metadata collection process.
        /// </summary>
        /// <param name="abandon">True to abandon the operation.</param>
        void Complete(bool abandon = false);
    }
}
