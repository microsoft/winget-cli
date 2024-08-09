// -----------------------------------------------------------------------------
// <copyright file="WinGetManifestException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for manifest operations.
    /// </summary>
    public class WinGetManifestException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetManifestException"/> class.
        /// </summary>
        public WinGetManifestException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetManifestException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetManifestException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetManifestException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetManifestException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetManifestException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetManifestException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
