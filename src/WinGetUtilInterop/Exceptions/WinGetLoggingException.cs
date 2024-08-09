// -----------------------------------------------------------------------------
// <copyright file="WinGetLoggingException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for logging operations.
    /// </summary>
    public class WinGetLoggingException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetLoggingException"/> class.
        /// </summary>
        public WinGetLoggingException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetLoggingException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetLoggingException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetLoggingException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetLoggingException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetLoggingException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetLoggingException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
