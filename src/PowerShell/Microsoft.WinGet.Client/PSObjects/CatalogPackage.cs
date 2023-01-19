// -----------------------------------------------------------------------------
// <copyright file="CatalogPackage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// CatalogPackage wrapper object for displaying to PowerShell.
    /// </summary>
    public class CatalogPackage
    {
        /// <summary>
        /// Gets or sets the name of the catalog package.
        /// </summary>
        public string Name { get; protected set; }

        /// <summary>
        /// Gets or sets the id of the catalog package.
        /// </summary>
        public string Id { get; protected set; }

        /// <summary>
        /// Gets or sets the version of the catalog package.
        /// </summary>
        public string Version { get; protected set; }

        /// <summary>
        /// Gets or sets a value indicating whether an update is available.
        /// </summary>
        public bool IsUpdateAvailable { get; protected set; }

        /// <summary>
        /// Gets or sets the source name of the catalog package.
        /// </summary>
        public string Source { get; protected set; }

        /// <summary>
        /// Gets or sets a list of strings representing the available versions.
        /// </summary>
        public string[] AvailableVersions { get; protected set; }
    }
}
