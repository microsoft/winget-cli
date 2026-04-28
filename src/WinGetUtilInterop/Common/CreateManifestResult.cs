// -----------------------------------------------------------------------------
// <copyright file="CreateManifestResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    using System;
    using System.Collections.Generic;
    using Microsoft.WinGetUtil.Interfaces;

    /// <summary>
    /// Class representing create manifest result.
    /// </summary>
    public class CreateManifestResult : IDisposable
    {
        private readonly IReadOnlyList<ManifestDiagnostic> diagnostics;
        private readonly bool hasDiagnostics;

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
            this.diagnostics = null;
            this.hasDiagnostics = false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="CreateManifestResult"/> class with structured diagnostics.
        /// </summary>
        /// <param name="isValid">Is manifest valid.</param>
        /// <param name="message">Warning or failure validation message (the full concatenated string).</param>
        /// <param name="manifestHandle">Created manifest handle if succeeded.</param>
        /// <param name="diagnostics">Structured list of individual errors and warnings.</param>
        public CreateManifestResult(bool isValid, string message, IWinGetManifest manifestHandle, IReadOnlyList<ManifestDiagnostic> diagnostics)
        {
            this.IsValid = isValid;
            this.Message = message;
            this.ManifestHandle = manifestHandle;
            this.diagnostics = diagnostics;
            this.hasDiagnostics = true;
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
        /// Gets the structured list of individual manifest errors and warnings.
        /// Only available when <see cref="WinGetCreateManifestOption.ReturnResponseAsJson"/> was passed
        /// to <see cref="Microsoft.WinGetUtil.Api.WinGetFactory.CreateManifest"/>. Accessing this
        /// property without that flag throws <see cref="InvalidOperationException"/>.
        /// </summary>
        /// <exception cref="InvalidOperationException">
        /// Thrown when <see cref="WinGetCreateManifestOption.ReturnResponseAsJson"/> was not set.
        /// </exception>
        public IReadOnlyList<ManifestDiagnostic> Diagnostics
        {
            get
            {
                if (!this.hasDiagnostics)
                {
                    throw new InvalidOperationException(
                        $"Structured diagnostics are not available. Pass {nameof(WinGetCreateManifestOption)}.{nameof(WinGetCreateManifestOption.ReturnResponseAsJson)} to CreateManifest to enable them.");
                }

                return this.diagnostics;
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
                if (this.ManifestHandle != null)
                {
                    this.ManifestHandle.Dispose();
                }
            }
        }
    }
}
