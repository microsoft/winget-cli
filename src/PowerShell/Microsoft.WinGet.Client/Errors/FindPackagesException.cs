﻿// -----------------------------------------------------------------------------
// <copyright file="FindPackagesException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Errors
{
    using System;
    using Microsoft.Management.Deployment;

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
            : base(@"There was an error searching for packages.")
        {
            this.Status = status;
        }

        /// <summary>
        /// Gets or sets the error status.
        /// </summary>
        public FindPackagesResultStatus Status { get; set; }
    }
}
