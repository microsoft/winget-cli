// -----------------------------------------------------------------------------
// <copyright file="WinGetCLITimeoutException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Time out exception for a winget cli command.
    /// </summary>
    public class WinGetCLITimeoutException : TimeoutException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetCLITimeoutException"/> class.
        /// </summary>
        /// <param name="command">Command.</param>
        /// <param name="parameters">Parameters.</param>
        public WinGetCLITimeoutException(string command, string? parameters)
            : base(string.Format(Resources.WinGetCLITimeoutExceptionMessage, command, parameters))
        {
        }
    }
}
