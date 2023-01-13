// -----------------------------------------------------------------------------
// <copyright file="FindPackagesException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Raised when there is an error searching for packages.
    /// </summary>
    [Serializable]
    public class FindPackagesException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FindPackagesException"/> class.
        /// </summary>
        /// <param name="status">A <see cref="FindPackagesResultStatus" /> value.</param>
        public FindPackagesException(FindPackagesResultStatus status)
            : base(string.Format(
                Resources.FindPackagesExceptionMessage,
                status.ToString()))
        {
            this.Status = status;
        }

        /// <summary>
        /// Gets the error status.
        /// </summary>
        public FindPackagesResultStatus Status { get; private set; }
    }
}
