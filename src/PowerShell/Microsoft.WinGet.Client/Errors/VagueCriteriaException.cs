// -----------------------------------------------------------------------------
// <copyright file="VagueCriteriaException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Errors
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// Raised when search criteria for installing or updating a package is too vague.
    /// </summary>
    [Serializable]
    public class VagueCriteriaException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VagueCriteriaException"/> class.
        /// </summary>
        /// <param name="one">The first <see cref="CatalogPackage" /> instance.</param>
        /// <param name="two">The second <see cref="CatalogPackage" /> instance.</param>
        public VagueCriteriaException(CatalogPackage one, CatalogPackage two)
            : base($"Both '{one.ToString(null)}' and '{two.ToString(null)}' match the input criteria. Please refine the input.")
        {
            this.PackageOne = one;
            this.PackageTwo = two;
        }

        /// <summary>
        /// Gets or sets the first conflicting package.
        /// </summary>
        public CatalogPackage PackageOne { get; set; }

        /// <summary>
        /// Gets or sets the second conflicting package.
        /// </summary>
        public CatalogPackage PackageTwo { get; set; }
    }
}
