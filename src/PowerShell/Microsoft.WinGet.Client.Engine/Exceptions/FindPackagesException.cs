// -----------------------------------------------------------------------------
// <copyright file="FindPackagesException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Raised when there is an error searching for packages.
    /// </summary>
    [Serializable]
    internal class FindPackagesException : RuntimeException
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
