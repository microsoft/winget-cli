// -----------------------------------------------------------------------------
// <copyright file="WinGetFactory.cs" company="Microsoft Corporation">
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
    /// Factory methods.
    /// </summary>
    public sealed class WinGetFactory : IWinGetFactory
    {
        private const uint IndexLatestVersion = unchecked((uint)-1);

        /// <inheritdoc/>
        public IWinGetSQLiteIndex SQLiteIndexCreate(string indexFile, uint majorVersion, uint minorVersion)
        {
            try
            {
                WinGetSQLiteIndexCreate(
                    indexFile,
                    majorVersion,
                    minorVersion,
                    out IntPtr index);
                return new WinGetSQLiteIndex(index);
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public IWinGetSQLiteIndex SQLiteIndexCreateLatestVersion(string indexFile)
        {
            return this.SQLiteIndexCreate(indexFile, IndexLatestVersion, IndexLatestVersion);
        }

        /// <inheritdoc/>
        public IWinGetSQLiteIndex SQLiteIndexOpen(string indexFile)
        {
            try
            {
                WinGetSQLiteIndexOpen(indexFile, out IntPtr index);
                return new WinGetSQLiteIndex(index);
            }
            catch (Exception e)
            {
                throw new WinGetSQLiteIndexException(e);
            }
        }

        /// <inheritdoc/>
        public IWinGetLogging LoggingInit(string indexLogFile)
        {
            try
            {
                WinGetLoggingInit(indexLogFile);
                return new WinGetLogging(indexLogFile);
            }
            catch (Exception e)
            {
                throw new WinGetLoggingException(e);
            }
        }

        /// <inheritdoc/>
        public CreateManifestResult CreateManifest(string manifestPath, string mergedManifestPath, WinGetCreateManifestOption option)
        {
            try
            {
                bool succeeded = false;
                IntPtr manifestHandle = IntPtr.Zero;
                string failureOrWarningMessage = null;
                WinGetCreateManifest(
                    manifestPath,
                    out succeeded,
                    out manifestHandle,
                    out failureOrWarningMessage,
                    mergedManifestPath,
                    option);

                // WinGetUtil uses exceptions for passing warning messages. When a warning is detected,
                // WinGetUtil will throw so a manifest handle will not be returned, in this case, we try
                // to create the manifest handle without any validation.
                if (succeeded && manifestHandle == IntPtr.Zero)
                {
                    WinGetCreateManifest(
                        manifestPath,
                        out succeeded,
                        out manifestHandle,
                        out string failureNotExpected,
                        null,
                        WinGetCreateManifestOption.NoValidation);
                }

                return new CreateManifestResult(succeeded, failureOrWarningMessage, succeeded ? new WinGetManifest(manifestHandle) : null);
            }
            catch (Exception e)
            {
                throw new WinGetManifestException(e);
            }
        }

        /// <summary>
        /// Begins the installer metadata collection process.
        /// </summary>
        /// <param name="input">input.</param>
        /// <param name="logFilePath">Log file path.</param>
        /// <param name="options">An enum of type <see cref="WinGetBeginInstallerMetadataCollectionOptions"/>.</param>
        /// <param name="outputFilePath">Metadata output file path.</param>
        /// <returns>IWinGetInstallerMetadata with the handle to the installer metadata collection.</returns>
        public IWinGetInstallerMetadata BeginInstallerMetadataCollection(
            string input,
            string logFilePath,
            WinGetBeginInstallerMetadataCollectionOptions options,
            string outputFilePath)
        {
            try
            {
                IntPtr collectionHandle = IntPtr.Zero;
                WinGetBeginInstallerMetadataCollection(
                    input,
                    logFilePath,
                    options,
                    out collectionHandle);

                return new WinGetInstallerMetadata(collectionHandle, outputFilePath);
            }
            catch (Exception e)
            {
                throw new WinGetInstallerMetadataException(e);
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
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        /// <summary>
        /// Opens an existing index at filePath.
        /// </summary>
        /// <param name="filePath">File path of index.</param>
        /// <param name="index">Out handle of the index.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetSQLiteIndexOpen(string filePath, out IntPtr index);

        /// <summary>
        /// Initializes the logging infrastructure.
        /// </summary>
        /// <param name="logPath">Log path.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetLoggingInit(string logPath);

        /// <summary>
        /// Creates a manifest with schema or semantic validation if needed.
        /// </summary>
        /// <param name="inputPath">Path of manifest.</param>
        /// <param name="succeeded">If create manifest succeeded.</param>
        /// <param name="manifestHandle">Out bool is validation succeeded.</param>
        /// <param name="failureMessage">Out string failure message, if any.</param>
        /// <param name="mergedManifestPath">Path to merged manifest file. Empty means no merged manifest needed.</param>
        /// <param name="option">Validate manifest option.</param>
        /// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetCreateManifest(
            string inputPath,
            [MarshalAs(UnmanagedType.U1)] out bool succeeded,
            out IntPtr manifestHandle,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            string mergedManifestPath,
            WinGetCreateManifestOption option);

        /// <summary>
        /// Begins the installer metadata collection process. By default, inputJSON is expected to be a JSON string.
        /// See the WinGetBeginInstallerMetadataCollectionOptions for more options.
        /// logFilePath optionally specifies where to write the log file for the collection operation.
        /// The collectionHandle is owned by the caller and must be passed to WinGetCompleteInstallerMetadataCollection to free it.
        /// </summary>
        /// <param name="inputJSON">Input Json.</param>
        /// <param name="logFilePath">Log file path.</param>
        /// <param name="options">An enum of type <see cref="WinGetBeginInstallerMetadataCollectionOptions"/>.</param>
        /// <param name="collectionHandle">Collection handle.</param>
        /// <returns>Metadata Collection handle.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetBeginInstallerMetadataCollection(
            string inputJSON,
            string logFilePath,
            WinGetBeginInstallerMetadataCollectionOptions options,
            out IntPtr collectionHandle);
    }
}
