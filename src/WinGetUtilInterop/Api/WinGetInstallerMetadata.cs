// -----------------------------------------------------------------------------
// <copyright file="WinGetInstallerMetadata.cs" company="Microsoft Corporation">
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
    /// Wrapper for WinGetInstallerMetadata operations.
    /// </summary>
    public sealed class WinGetInstallerMetadata : IWinGetInstallerMetadata
    {
        private readonly string outputFilePath;
        private IntPtr collectionHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetInstallerMetadata"/> class.
        /// </summary>
        /// <param name="collectionHandle">Collection handle.</param>
        /// <param name="outputFilePath">Output file path.</param>
        internal WinGetInstallerMetadata(IntPtr collectionHandle, string outputFilePath)
        {
            this.collectionHandle = collectionHandle;
            this.outputFilePath = outputFilePath;
        }

        /// <inheritdoc />
        public void Complete(bool abandon = false)
        {
            try
            {
                this.CompleteInternal(abandon ?
                    WinGetCompleteInstallerMetadataCollectionOptions.WinGetCompleteInstallerMetadataCollectionOption_Abandon :
                    WinGetCompleteInstallerMetadataCollectionOptions.WinGetCompleteInstallerMetadataCollectionOption_None);
                this.collectionHandle = IntPtr.Zero;
            }
            catch (Exception e)
            {
                throw new WinGetInstallerMetadataException(e);
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
        /// Dispose method to free the manifest handle.
        /// </summary>
        /// <param name="disposing">Bool value indicating if Dispose is being run.</param>
        public void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.Complete(true);
            }
        }

        /// <summary>
        /// Completes the installer metadata collection process, Always frees the collectionHandle.
        /// WinGetCompleteInstallerMetadataCollection must be called exactly once for each call to WinGetBeginInstallerMetadataCollection.
        /// </summary>
        /// <param name="collectionHandle">Collection Handle.</param>
        /// <param name="outputFilePath">Metadata output file path.</param>
        /// <param name="options">An enum of type <see cref="WinGetBeginInstallerMetadataCollectionOptions"/>.</param>
        /// <returns>Metadata collection handle.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetCompleteInstallerMetadataCollection(
            IntPtr collectionHandle,
            string outputFilePath,
            WinGetCompleteInstallerMetadataCollectionOptions options);

        private void CompleteInternal(WinGetCompleteInstallerMetadataCollectionOptions options)
        {
            if (this.collectionHandle != IntPtr.Zero)
            {
                WinGetCompleteInstallerMetadataCollection(
                    this.collectionHandle,
                    this.outputFilePath,
                    options);
            }
        }
    }
}
