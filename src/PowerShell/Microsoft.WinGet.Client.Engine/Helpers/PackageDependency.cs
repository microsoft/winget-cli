// -----------------------------------------------------------------------------
// <copyright file="PackageDependency.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace Microsoft.WinGet.Client.Engine.Helpers
{
    /// <summary>
    /// A single package dependency.
    /// </summary>
    internal class PackageDependency
    {
        /// <summary>
        /// Gets or sets the name of the dependency.
        /// </summary>
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the version of the dependency.
        /// </summary>
        public string Version { get; set; } = string.Empty;
    }
}
