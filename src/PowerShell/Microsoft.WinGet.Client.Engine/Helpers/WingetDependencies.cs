// -----------------------------------------------------------------------------
// <copyright file="WingetDependencies.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Collections.Generic;

    /// <summary>
    /// An object representation of the dependencies json.
    /// </summary>
    internal class WingetDependencies
    {
        /// <summary>
        /// Gets or sets a list of required package dependencies.
        /// </summary>
        public List<PackageDependency> Dependencies { get; set; } = new List<PackageDependency>();
    }
}
