// -----------------------------------------------------------------------------
// <copyright file="WinGetDownloadException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for download operations.
    /// </summary>
    public class WinGetDownloadException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloadException"/> class.
        /// </summary>
        public WinGetDownloadException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloadException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetDownloadException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloadException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetDownloadException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetDownloadException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetDownloadException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
