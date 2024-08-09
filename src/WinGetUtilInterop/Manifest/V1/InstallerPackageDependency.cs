// -----------------------------------------------------------------------------
// <copyright file="InstallerPackageDependency.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Class that contains package dependency from same source.
    /// </summary>
    public class InstallerPackageDependency
    {
        /// <summary>
        /// Gets or sets the dependency package identifier.
        /// </summary>
        public string PackageIdentifier { get; set; }

        /// <summary>
        /// Gets or sets the minimum version of the dependency package.
        /// </summary>
        public string MinimumVersion { get; set; }
    }
}
