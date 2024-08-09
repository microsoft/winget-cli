// -----------------------------------------------------------------------------
// <copyright file="WinGetInstallerMetadataException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Exceptions
{
    using System;

    /// <summary>
    /// Exception for installer metadata operations.
    /// </summary>
    public class WinGetInstallerMetadataException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetInstallerMetadataException"/> class.
        /// </summary>
        public WinGetInstallerMetadataException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetInstallerMetadataException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public WinGetInstallerMetadataException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetInstallerMetadataException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public WinGetInstallerMetadataException(Exception inner)
            : base(string.Empty, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetInstallerMetadataException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public WinGetInstallerMetadataException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
