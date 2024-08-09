// -----------------------------------------------------------------------------
// <copyright file="InstallerDependency.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Class that contains different dependencies of installer.
    /// </summary>
    public class InstallerDependency
    {
        /// <summary>
        /// Gets or sets the windows features installer depends on.
        /// </summary>
        public List<string> WindowsFeatures { get; set; }

        /// <summary>
        /// Gets or sets the windows libraries installer depends on.
        /// </summary>
        public List<string> WindowsLibraries { get; set; }

        /// <summary>
        /// Gets or sets the package dependencies under same source.
        /// </summary>
        public List<InstallerPackageDependency> PackageDependencies { get; set; }

        /// <summary>
        /// Gets or sets the external dependencies installer depends on.
        /// </summary>
        public List<string> ExternalDependencies { get; set; }
    }
}
