// -----------------------------------------------------------------------------
// <copyright file="WinGetSQLiteIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Api
{
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Exceptions;
    using Microsoft.WinGetUtil.Interfaces;

    /// <summary>
    /// Wrapper class for SQLite index operations.
    /// </summary>
    public sealed class WinGetSQLiteIndex : IWinGetSQLiteIndex
    {
        private readonly IntPtr indexHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetSQLiteIndex"/> class.
        /// </summary>
        /// <param name="indexHandle">Handle of the index.</param>
        internal WinGetSQLiteIndex(IntPtr indexHandle)
        {
            this.indexHandle = indexHandle;
        }

        /// <inheritdoc/>
        public void MigrateTo(uint majorVersion, uint minorVersion)
        {
            try
            {
                WinGetSQLiteIndexMigrate(this.indexHandle, majorVersion, minorVersion);
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public void SetProperty(SQLiteIndexProperty property, string value)
        {
            try
            {
                WinGetSQLiteIndexSetProperty(this.indexHandle, property, value);
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public void AddManifest(string manifestPath, string relativePath)
        {
            try
            {
                WinGetSQLiteIndexAddManifest(this.indexHandle, manifestPath, relativePath);
                return;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public bool UpdateManifest(string manifestPath, string relativePath)
        {
            try
            {
                // For now, modifying a manifest implies that the file didn't got moved in the repository. So only
                // contents of the file are modified. However, in the future we might support moving which requires
                // oldManifestPath, oldRelativePath, newManifestPath and oldManifestPath.
                WinGetSQLiteIndexUpdateManifest(
                    this.indexHandle,
                    manifestPath,
                    relativePath,
                    out bool indexModified);
                return indexModified;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public bool AddOrUpdateManifest(string manifestPath, string relativePath)
        {
            try
            {
                // For now, modifying a manifest implies that the file didn't got moved in the repository. So only
                // contents of the file are modified. However, in the future we might support moving which requires
                // oldManifestPath, oldRelativePath, newManifestPath and oldManifestPath.
                WinGetSQLiteIndexAddOrUpdateManifest(
                    this.indexHandle,
                    manifestPath,
                    relativePath,
                    out bool indexModified);
                return indexModified;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public void RemoveManifest(string manifestPath, string relativePath)
        {
            try
            {
                WinGetSQLiteIndexRemoveManifest(this.indexHandle, manifestPath, relativePath);
                return;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public void PrepareForPackaging()
        {
            try
            {
                WinGetSQLiteIndexPrepareForPackaging(this.indexHandle);
                return;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public bool IsIndexConsistent()
        {
            try
            {
                WinGetSQLiteIndexCheckConsistency(this.indexHandle, out bool indexModified);
                return indexModified;
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public IntPtr GetIndexHandle()
        {
            return this.indexHandle;
        }

        /// <summary>
        /// Dispose method.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose method to free the sqlite index handle.
        /// </summary>
        /// <param name="disposing">Bool value indicating if Dispose is being run.</param>
        public void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.indexHandle != IntPtr.Zero)
                {
                    WinGetSQLiteIndexClose(this.indexHandle);
                }
            }
        }

        /// <summary>
        /// Migrates the index to the target version.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexMigrate(IntPtr index, uint majorVersion, uint minorVersion);

        /// <summary>
        /// Sets a property on the index.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="property">The property to set.</param>
        /// <param name="value">The value to set.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexSetProperty(IntPtr index, SQLiteIndexProperty property, string value);

        /// <summary>
        /// Closes the index.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexClose(IntPtr index);

        /// <summary>
        /// Adds the manifest at the repository relative path to the index.
        /// If the function succeeds, the manifest has been added.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Manifest to add.</param>
        /// <param name="relativePath">Path of the manifest in the container.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexAddManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// Updates the manifest at the repository relative path in the index.
        /// The out value indicates whether the index was modified by the function.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Old manifest path.</param>
        /// <param name="relativePath">Old relative path in the container.</param>
        /// <param name="indexModified">Out bool if the index is modified.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexUpdateManifest(
            IntPtr index,
            string manifestPath,
            string relativePath,
            [MarshalAs(UnmanagedType.U1)] out bool indexModified);

        /// <summary>
        /// Adds or Updates the manifest at the repository relative path in the index.
        /// The out value indicates whether the index was modified by the function.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="relativePath">Relative path in the container.</param>
        /// <param name="indexModified">Out bool if the index is modified.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexAddOrUpdateManifest(
            IntPtr index,
            string manifestPath,
            string relativePath,
            [MarshalAs(UnmanagedType.U1)] out bool indexModified);

        /// <summary>
        /// Removes the manifest at the repository relative path from the index.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <param name="manifestPath">Manifest path to remove.</param>
        /// <param name="relativePath">Relative path in the container.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// Removes data that is no longer needed for an index that is to be published.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexPrepareForPackaging(IntPtr index);

        /// <summary>
        /// Checks the index for consistency, ensuring that at a minimum all referenced rows actually exist.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <param name="succeeded">Does the consistency check succeeded.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexCheckConsistency(IntPtr index, [MarshalAs(UnmanagedType.U1)] out bool succeeded);
    }
}
