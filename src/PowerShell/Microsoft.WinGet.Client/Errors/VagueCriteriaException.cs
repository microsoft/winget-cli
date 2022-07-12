// -----------------------------------------------------------------------------
// <copyright file="VagueCriteriaException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Errors
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Helpers;

    /// <summary>
    /// Raised when search criteria for installing or updating a package is too vague.
    /// </summary>
    [Serializable]
    public class VagueCriteriaException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VagueCriteriaException"/> class.
        /// </summary>
        /// <param name="results">The list of conflicting packages of length at least two.</param>
        public VagueCriteriaException(IReadOnlyList<MatchResult> results)
            : base(string.Format(
                Constants.ResourceManager.GetString("ExceptionMessages_VagueSearch"),
                (results.Count > 0) ? results[0].CatalogPackage.ToString(null) : null,
                (results.Count > 1) ? results[1].CatalogPackage.ToString(null) : null,
                Math.Max(results.Count, 2) - 2))
        {
            this.MatchResults = results;
        }

        /// <summary>
        /// Gets or sets the list of conflicting packages.
        /// </summary>
        public IReadOnlyList<MatchResult> MatchResults { get; set; }
    }
}
