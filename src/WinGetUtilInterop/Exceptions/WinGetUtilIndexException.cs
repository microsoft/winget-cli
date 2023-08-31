// -----------------------------------------------------------------------
// <copyright file="WinGetUtilSQLiteIndexException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
// -----------------------------------------------------------------------

namespace WinGetUtilInterop.Exceptions
{
    using System;

    public class WinGetUtilIndexException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetUtilIndexException"/> class.
        /// </summary>
        public WinGetUtilIndexException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetUtilIndexException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetUtilIndexException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetUtilIndexException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetUtilIndexException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetUtilIndexException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetUtilIndexException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
