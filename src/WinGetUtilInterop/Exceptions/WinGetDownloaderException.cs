// -----------------------------------------------------------------------------
// <copyright file="WinGetDownloaderException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for download operations.
    /// </summary>
    public class WinGetDownloaderException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloaderException"/> class.
        /// </summary>
        public WinGetDownloaderException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloaderException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetDownloaderException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloaderException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetDownloaderException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloaderException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetDownloaderException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
