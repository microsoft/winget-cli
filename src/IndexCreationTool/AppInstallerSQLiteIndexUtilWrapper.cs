// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexCreationTool
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Wrapper class around AppInstallerSQLiteIndexUtil index modifier native implementation.
    /// </summary>
    internal class AppInstallerSQLiteIndexUtilWrapper : IDisposable
    {
        /// <summary>
        /// Dll name.
        /// </summary>
        public const string DllName = @"AppInstallerSQLiteIndexUtil.dll";

        private const uint LatestVersion = unchecked((uint)-1);

        private IntPtr indexHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppInstallerSQLiteIndexUtilWrapper"/> class.
        /// </summary>
        /// <param name="indexHandle">Handle of the index.</param>
        private AppInstallerSQLiteIndexUtilWrapper(IntPtr indexHandle)
        {
            this.indexHandle = indexHandle;
        }

        /// <summary>
        /// Creates a new index file in the specified path.
        /// </summary>
        /// <param name="indexFile">Index file to create.</param>
        /// <returns>Instance of AppInstallerSQLiteIndexUtilWrapper.</returns>
        public static AppInstallerSQLiteIndexUtilWrapper Create(string indexFile)
        {
            try
            {
                AppInstallerSQLiteIndexCreate(
                    indexFile,
                    AppInstallerSQLiteIndexUtilWrapper.LatestVersion,
                    AppInstallerSQLiteIndexUtilWrapper.LatestVersion,
                    out IntPtr index);
                return new AppInstallerSQLiteIndexUtilWrapper(index);
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to create {indexFile}. {Environment.NewLine}{e.ToString()}");
                throw;
            }
        }

        /// <summary>
        /// Open the index.
        /// </summary>
        /// <param name="indexFile">Index file to open.</param>
        /// <returns>Instance of AppInstallerSQLiteIndexUtilWrapper.</returns>
        public static AppInstallerSQLiteIndexUtilWrapper Open(string indexFile)
        {
            try
            {
                AppInstallerSQLiteIndexOpen(indexFile, out IntPtr index);
                return new AppInstallerSQLiteIndexUtilWrapper(index);
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to open {indexFile}. {Environment.NewLine}{e.ToString()}");
                throw;
            }
        }

        /// <summary>
        /// Adds manifest to index.
        /// </summary>
        /// <param name="manifestPath">Manifest to add.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        public void AddManifest(string manifestPath, string relativePath)
        {
            try
            {
                Console.WriteLine($"Adding manifest {manifestPath} on index file.");
                AppInstallerSQLiteIndexAddManifest(this.indexHandle, manifestPath, relativePath);
                return;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to add manifest {manifestPath} with relative path {relativePath}. {Environment.NewLine}{e.ToString()}");
                throw;
            }
        }

        /// <summary>
        /// Updates manifest in the index.
        /// </summary>
        /// <param name="manifestPath">Path to manifest to modify.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        /// <returns>True if index was modified.</returns>
        public bool UpdateManifest(string manifestPath, string relativePath)
        {
            try
            {
                Console.WriteLine($"Updating manifest {manifestPath} on index file.");

                // For now, modifying a manifest implies that the file didn't got moved in the repository. So only
                // contents of the file are modified. However, in the future we might support moving which requires
                // oldManifestPath, oldRelativePath, newManifestPath and oldManifestPath.
                AppInstallerSQLiteIndexUpdateManifest(
                    this.indexHandle,
                    manifestPath,
                    relativePath,
                    out bool indexModified);

                if (!indexModified)
                {
                    // This means that some of the attributes that don't get indexed, like Description, where
                    // were modified, so the index doesn't got updated.
                    Console.WriteLine($"Manifest {manifestPath} didn't result in a modification to the index.");
                }

                return indexModified;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to update manifest {manifestPath} with relative path {relativePath}. {Environment.NewLine}{e.ToString()}");
                throw;
            }
}

        /// <summary>
        /// Delete manifest from index.
        /// </summary>
        /// <param name="manifestPath">Path to manifest to modify.</param>
        /// <param name="relativePath">Path of the manifest in the repository.</param>
        public void RemoveManifest(string manifestPath, string relativePath)
        {
            try
            {
                AppInstallerSQLiteIndexRemoveManifest(this.indexHandle, manifestPath, relativePath);
                return;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to remove manifest {manifestPath} with relative path {relativePath}. {Environment.NewLine}{e.ToString()}");
                throw;
            }
        }

        /// <summary>
        /// Wrapper for AppInstallerSQLiteIndexPrepareForPackaging.
        /// </summary>
        public void PrepareForPackaging()
        {
            try
            {
                AppInstallerSQLiteIndexPrepareForPackaging(this.indexHandle);
                return;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error to prepare for packaging. {Environment.NewLine}{e.ToString()}");
                throw;
            }
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
        /// Dispose method to dispose the Git desktop process runner.
        /// </summary>
        /// <param name="disposing">Bool value indicating if Dispose is being run.</param>
        protected void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.indexHandle != null)
                {
                    AppInstallerSQLiteIndexClose(this.indexHandle);
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
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        /// <summary>
        /// Opens an existing index at filePath.
        /// </summary>
        /// <param name="filePath">File path of index.</param>
        /// <param name="index">Out handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexOpen(string filePath, out IntPtr index);

        /// <summary>
        /// Closes the index.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexClose(IntPtr index);

        /// <summary>
        /// Adds the manifest at the repository relative path to the index.
        /// If the function succeeds, the manifest has been added.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Manifest to add.</param>
        /// <param name="relativePath">Path of the manifest in the container.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexAddManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// Updates the manifest at the repository relative path in the index.
        /// The out value indicates whether the index was modified by the function.
        /// </summary>
        /// <param name="index">Handle of the index.</param>
        /// <param name="manifestPath">Old manifest path.</param>
        /// <param name="relativePath">Old relative path in the container.</param>
        /// <param name="indexModified">Out bool if the index is modified.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexUpdateManifest(
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
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// Removes data that is no longer needed for an index that is to be published.
        /// </summary>
        /// <param name="index">Index handle.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr AppInstallerSQLiteIndexPrepareForPackaging(IntPtr index);
    }
}
