// -----------------------------------------------------------------------------
// <copyright file="CreateManifestResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    using System;
    using Microsoft.WinGetUtil.Interfaces;

    /// <summary>
    /// Class representing create manifest result.
    /// </summary>
    public class CreateManifestResult : IDisposable
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CreateManifestResult"/> class.
        /// </summary>
        /// <param name="isValid">Is manifest valid.</param>
        /// <param name="message">Warning or failure validation message.</param>
        /// <param name="manifestHandle">Created manifest handle if succeeded.</param>
        public CreateManifestResult(bool isValid, string message, IWinGetManifest manifestHandle)
        {
            this.IsValid = isValid;
            this.Message = message;
            this.ManifestHandle = manifestHandle;
        }

        /// <summary>
        /// Gets a value indicating whether the manifest is valid.
        /// </summary>
        public bool IsValid { get; } = false;

        /// <summary>
        /// Gets warning or failure validation message.
        /// </summary>
        public string Message { get; } = null;

        /// <summary>
        /// Gets created manifest handle if succeeded.
        /// </summary>
        public IWinGetManifest ManifestHandle { get; } = null;

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
                if (this.ManifestHandle != null)
                {
                    this.ManifestHandle.Dispose();
                }
            }
        }
    }
}
