// -----------------------------------------------------------------------------
// <copyright file="VagueCriteriaException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Raised when search criteria for installing or updating a package is too vague.
    /// </summary>
    [Serializable]
    internal class VagueCriteriaException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VagueCriteriaException"/> class.
        /// </summary>
        /// <param name="results">The list of conflicting packages of length at least two.</param>
        public VagueCriteriaException(IReadOnlyList<MatchResult> results)
            : base(string.Format(
                Resources.VagueCriteriaExceptionMessage,
                results[0].CatalogPackage.ToString(null),
                results[1].CatalogPackage.ToString(null),
                results.Count - 2))
        {
            this.MatchResults = results;
        }

        /// <summary>
        /// Gets the list of conflicting packages.
        /// </summary>
        public IReadOnlyList<MatchResult> MatchResults { get; private set; }
    }
}
