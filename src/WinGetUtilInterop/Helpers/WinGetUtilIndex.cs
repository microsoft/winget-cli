// -----------------------------------------------------------------------
// <copyright file="WinGetUtilWrapperSQLiteIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
// -----------------------------------------------------------------------

namespace WinGetUtilInterop.Helpers
{
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.WinGetUtil.Interfaces;
    using WinGetUtilInterop.Exceptions;

    public class WinGetUtilIndex : IWinGetUtilIndex
    {
        private const string WinGetUtilDll = "WinGetUtil.dll";
        private const uint IndexLatestVersion = unchecked((uint)-1);

        /// <summary>
        /// WinGet Index latest version.
        /// </summary>
        public const uint WinGetIndexLatestVersion = unchecked((uint)-1);

        private readonly IntPtr indexHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetUtilIndex"/> class.
        /// </summary>
        /// <param name="indexHandle">Handle of the index.</param>
        /// <param name="loggingContext">Logging Context.</param>
        private WinGetUtilIndex(IntPtr indexHandle)
        {
            this.indexHandle = indexHandle;
        }

        /// <summary>
        /// Creates a new index file in the specified path.
        /// </summary>
        /// <param name="indexFile">Index file to create.</param>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        /// <param name="loggingContext">Logging Context.</param>
        /// <returns>Instance of IWinGetUtilSQLiteIndex.</returns>
        public static IWinGetUtilIndex Create(string indexFile, uint majorVersion, uint minorVersion)
        {
            try
            {
                WinGetSQLiteIndexCreate(
                    indexFile,
                    majorVersion,
                    minorVersion,
                    out IntPtr index);
                return new WinGetUtilIndex(index);
            }
            catch (Exception e)
            {
                throw new WinGetUtilIndexException(e);
            }
        }

        /// <summary>
        /// Creates a new index file in the specified path.
        /// </summary>
        /// <param name="indexFile">Index file to create.</param>
        /// <returns>Instance of IWinGetUtilSQLiteIndex.</returns>
        public static IWinGetUtilIndex CreateLatestVersion(string indexFile)
        {
            return Create(indexFile, IndexLatestVersion, IndexLatestVersion);
        }

        /// <summary>
        /// Open the index file.
        /// </summary>
        /// <param name="indexFile">Index file to open.</param>
        /// <returns>Instance of IWinGetUtilSQLiteIndex.</returns>
        public static IWinGetUtilIndex Open(string indexFile)
        {
            try
            {
                WinGetSQLiteIndexOpen(indexFile, out IntPtr index);
                return new WinGetUtilIndex(index);
            }
            catch (Exception e)
            {
                throw new WinGetUtilIndexException(e);
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
                throw new WinGetUtilIndexException(e);
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
                throw new WinGetUtilIndexException(e);
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
                throw new WinGetUtilIndexException(e);
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
                throw new WinGetUtilIndexException(e);
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
                throw new WinGetUtilIndexException(e);
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
                if (this.indexHandle != null)
                {
                    WinGetSQLiteIndexClose(this.indexHandle);
                }
            }
        }

        /// <summary>
        /// Creates a new index file at filePath with the given version.
        /// </summary>
        /// <param name="filePath">File path to create index.</param>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        /// <param name="index">Out handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        /// <summary>
        /// Opens an existing index at filePath.
        /// </summary>
        /// <param name="filePath">File path of index.</param>
        /// <param name="index">Out handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexOpen(string filePath, out IntPtr index);

        /// <summary>
        /// Closes the index.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexClose(IntPtr index);

        /// <summary>
        /// Adds the manifest at the repository relative path to the index.
        /// If the function succeeds, the manifest has been added.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Manifest to add.</param>
        /// <param name="relativePath">Path of the manifest in the container.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
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
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexUpdateManifest(
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
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// Removes data that is no longer needed for an index that is to be published.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexPrepareForPackaging(IntPtr index);

        /// <summary>
        /// Checks the index for consistency, ensuring that at a minimum all referenced rows actually exist.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <param name="succeeded">Does the consistency check succeeded.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(WinGetUtilDll, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexCheckConsistency(IntPtr index, [MarshalAs(UnmanagedType.U1)] out bool succeeded);
    }
}
