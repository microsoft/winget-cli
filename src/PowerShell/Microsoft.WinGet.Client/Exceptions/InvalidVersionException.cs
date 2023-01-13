// -----------------------------------------------------------------------------
// <copyright file="InvalidVersionException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Invalid version.
    /// </summary>
    [Serializable]
    public class InvalidVersionException : ArgumentException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvalidVersionException"/> class.
        /// </summary>
        /// <param name="version">Version.</param>
        public InvalidVersionException(string version)
            : base(string.Format(Resources.InvalidVersionExceptionMessage, version))
        {
            this.Version = version;
        }

        /// <summary>
        /// Gets the version.
        /// </summary>
        public string Version { get; private set; }
    }
}
