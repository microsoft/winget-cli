// -----------------------------------------------------------------------
// <copyright file="WinGetUtilWrapperManifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    using System;

    public interface IWinGetUtilIndex : IDisposable
    {
        /// <summary>
        /// Adds manifest to index.
        /// </summary>
        /// <param name="manifestPath">Manifest to add.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        void AddManifest(string manifestPath, string relativePath);

        /// <summary>
        /// Updates manifest in the index.
        /// </summary>
        /// <param name="manifestPath">Path to manifest to modify.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        /// <returns>True if index was modified.</returns>
        bool UpdateManifest(string manifestPath, string relativePath);

        /// <summary>
        /// Delete manifest from index.
        /// </summary>
        /// <param name="manifestPath">Path to manifest to modify.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        void RemoveManifest(string manifestPath, string relativePath);

        /// <summary>
        /// Wrapper for WinGetSQLiteIndexPrepareForPackaging.
        /// </summary>
        void PrepareForPackaging();

        /// <summary>
        /// Checks the index for consistency, ensuring that at a minimum all referenced rows actually exist.
        /// </summary>
        /// <returns>Is index consistent.</returns>
        bool IsIndexConsistent();

        /// <summary>
        /// Gets the managed index handle. It is used in additional manifest validation that requires an index.
        /// </summary>
        /// <returns>The managed index handle.</returns>
        IntPtr GetIndexHandle();
    }
}
