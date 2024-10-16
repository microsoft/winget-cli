// -----------------------------------------------------------------------------
// <copyright file="IWinGetSQLiteIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    using System;

    /// <summary>
    /// The properties that can be set with IWinGetSQLiteIndex::SetProperty.
    /// The values must match those in WinGetUtil.h.
    /// </summary>
    public enum SQLiteIndexProperty
    {
        /// <summary>
        /// The base time to use for update tracking. The value is in the Unix epoch.
        /// Set to an empty string to use the current time.
        /// Set to 0 to force all files to be output.
        /// </summary>
        PackageUpdateTrackingBaseTime = 0,

        /// <summary>
        /// The full path to a base directory where intermediate files will be output.
        /// The path does not need to exist, and may not be created if no files need to be written.
        /// </summary>
        IntermediateFileOutputPath = 1,
    }

    /// <summary>
    /// Interface for index operations.
    /// </summary>
    public interface IWinGetSQLiteIndex : IDisposable
    {
        /// <summary>
        /// Migrates the index to the given version.
        /// </summary>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        void MigrateTo(uint majorVersion, uint minorVersion);

        /// <summary>
        /// Sets the given property to the given value.
        /// </summary>
        /// <param name="property">The property to set.</param>
        /// <param name="value">The value to set.</param>
        void SetProperty(SQLiteIndexProperty property, string value);

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
        /// Adds or Updates manifest in the index.
        /// </summary>
        /// <param name="manifestPath">Path to manifest.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        /// <returns>True if added; false if updated.</returns>
        bool AddOrUpdateManifest(string manifestPath, string relativePath);

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
