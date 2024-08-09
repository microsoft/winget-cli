// -----------------------------------------------------------------------------
// <copyright file="IWinGetFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    using Microsoft.WinGetUtil.Common;

    /// <summary>
    /// Factory methods.
    /// </summary>
    public interface IWinGetFactory
    {
        /// <summary>
        /// Creates a new index file in the specified path.
        /// </summary>
        /// <param name="indexFile">Index file to create.</param>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        /// <returns>Instance of IWinGetSQLiteIndex.</returns>
        IWinGetSQLiteIndex SQLiteIndexCreate(string indexFile, uint majorVersion, uint minorVersion);

        /// <summary>
        /// Creates a new index file in the specified path.
        /// </summary>
        /// <param name="indexFile">Index file to create.</param>
        /// <returns>Instance of IWinGetSQLiteIndex.</returns>
        IWinGetSQLiteIndex SQLiteIndexCreateLatestVersion(string indexFile);

        /// <summary>
        /// Open the index file.
        /// </summary>
        /// <param name="indexFile">Index file to open.</param>
        /// <returns>Instance of IWinGetSQLiteIndex.</returns>
        IWinGetSQLiteIndex SQLiteIndexOpen(string indexFile);

        /// <summary>
        /// Initializes logging.
        /// </summary>
        /// <param name="logFile">Log index file.</param>
        /// <returns>Instance of IWinGetLogging.</returns>
        IWinGetLogging LoggingInit(string logFile);

        /// <summary>
        /// Validates the manifest is compliant and creates a manifest handle if succeeded.
        /// </summary>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="mergedManifestPath">Merged manifest output path.</param>
        /// <param name="option">Desired validate manifest option.</param>
        /// <returns>The <see cref="CreateManifestResult"/> class.</returns>
        CreateManifestResult CreateManifest(string manifestPath, string mergedManifestPath, WinGetCreateManifestOption option);
    }
}
