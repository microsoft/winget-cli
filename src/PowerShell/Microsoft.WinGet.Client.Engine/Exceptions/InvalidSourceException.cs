// -----------------------------------------------------------------------------
// <copyright file="InvalidSourceException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Invalid source.
    /// </summary>
    [Serializable]
    public class InvalidSourceException : ArgumentException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvalidSourceException"/> class.
        /// </summary>
        /// <param name="sourceName">Source name.</param>
        public InvalidSourceException(string sourceName)
            : base(string.Format(Resources.InvalidSourceExceptionMessage, sourceName))
        {
            this.SourceName = sourceName;
        }

        /// <summary>
        /// Gets the source name.
        /// </summary>
        public string SourceName { get; private set; }
    }
}
