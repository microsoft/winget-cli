// -----------------------------------------------------------------------------
// <copyright file="WinGetSQLiteIndexException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for SQLite index operations.
    /// </summary>
    public class WinGetSQLiteIndexException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetSQLiteIndexException"/> class.
        /// </summary>
        public WinGetSQLiteIndexException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetSQLiteIndexException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetSQLiteIndexException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetSQLiteIndexException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetSQLiteIndexException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetSQLiteIndexException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetSQLiteIndexException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
