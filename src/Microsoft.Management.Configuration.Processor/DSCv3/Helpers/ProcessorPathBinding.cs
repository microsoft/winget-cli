// -----------------------------------------------------------------------------
// <copyright file="ProcessorPathBinding.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Win32.SafeHandles;

    /// <summary>
    /// Verifies the processor path once and then pins it for the lifetime of the object, so that the
    /// verified file is the file that gets executed every time; no further checks are required.
    /// <para>
    /// The pin has two parts:
    /// </para>
    /// <list type="number">
    /// <item><description>
    /// A handle to the file itself, opened sharing read only and without following reparse points.
    /// Sharing read still allows the file to be executed, but denies every other opener write and
    /// delete access. Since renaming a file requires delete access, the verified bytes cannot be
    /// modified, replaced, renamed or deleted. If the path is a link, the link target is pinned the
    /// same way; a link cannot be retargeted without write access to it, so holding both ends means
    /// the path cannot be made to resolve to a different file.
    /// </description></item>
    /// <item><description>
    /// Handles to each ancestor directory of the file's normalized path, opened sharing read and
    /// write but not delete. That prevents any component of the path from being renamed or deleted,
    /// which would otherwise allow a different file to be placed at the same path without ever
    /// touching the pinned file. Creating and deleting files inside those directories is unaffected.
    /// </description></item>
    /// </list>
    /// <para>
    /// The path used to launch the processor is the normalized path obtained from the pinned handle,
    /// which contains no symbolic links or junctions. No component of it can be re-pointed at another
    /// target afterwards either: setting a reparse point on a directory requires it to be empty, and
    /// every pinned directory permanently contains the pinned component below it. Each directory is
    /// also opened without following reparse points and rejected if it is one, so a component that is
    /// swapped for a link while the path is being pinned is detected rather than followed.
    /// </para>
    /// </summary>
    internal sealed class ProcessorPathBinding : IDisposable
    {
        private const int MaximumPinAttempts = 3;

        private readonly object bindingLock = new ();

        private string? boundPath = null;
        private bool boundIsAlias = false;
        private PinnedProcessorFile? pinnedFile = null;
        private List<SafeFileHandle> directoryHandles = new ();
        private string? launchPath = null;
        private bool disposed = false;

        /// <summary>
        /// Gets a value indicating whether the path has been verified and pinned.
        /// </summary>
        public bool IsPinned
        {
            get
            {
                lock (this.bindingLock)
                {
                    return this.launchPath != null;
                }
            }
        }

        /// <summary>
        /// Verifies and pins the given path on first use, returning the path to use to launch the
        /// processor. Subsequent calls return the already pinned path.
        /// </summary>
        /// <param name="path">The path to the DSC executable or app execution alias.</param>
        /// <param name="expectedHash">The expected SHA256 hash (hex string, case-insensitive).</param>
        /// <param name="isAlias">Whether the path is an app execution alias reparse point.</param>
        /// <returns>The pinned path to launch the processor with.</returns>
        public string EnsurePinned(string path, string expectedHash, bool isAlias)
        {
            lock (this.bindingLock)
            {
                ObjectDisposedException.ThrowIf(this.disposed, this);

                if (this.launchPath == null)
                {
                    this.Pin(path, expectedHash, isAlias);
                }
                else if (!string.Equals(this.boundPath, path, StringComparison.OrdinalIgnoreCase) ||
                         this.boundIsAlias != isAlias)
                {
                    // The pin covers exactly one target; a different one has not been verified.
                    throw new DscProcessorPathChangedException();
                }

                return this.launchPath!;
            }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            lock (this.bindingLock)
            {
                if (!this.disposed)
                {
                    this.ReleaseDirectoryHandles();
                    this.pinnedFile?.Dispose();
                    this.pinnedFile = null;
                    this.launchPath = null;
                    this.disposed = true;
                }
            }
        }

        private void Pin(string path, string expectedHash, bool isAlias)
        {
            PinnedProcessorFile file = ProcessorPathIntegrity.VerifyAndOpen(path, expectedHash, isAlias);

            try
            {
                string finalPath = ProcessorPathIntegrity.GetFinalPath(file.ContentHandle);

                // Pinning the ancestors is itself done by path, so a rename that happens while the
                // ancestors are being pinned could leave a directory pinned that is no longer part of
                // the path. Re-reading the final path from the pinned file handle afterwards detects
                // that; once the final path is stable across the pinning, no component of it can be
                // renamed or deleted any longer.
                for (int attempt = 0; ; ++attempt)
                {
                    try
                    {
                        this.PinAncestors(finalPath);

                        string currentPath = ProcessorPathIntegrity.GetFinalPath(file.ContentHandle);
                        if (string.Equals(currentPath, finalPath, StringComparison.OrdinalIgnoreCase))
                        {
                            break;
                        }

                        finalPath = currentPath;
                    }
                    catch (DscProcessorPathChangedException) when (attempt + 1 < MaximumPinAttempts)
                    {
                        // A component of the path was replaced with a reparse point while the
                        // ancestors were being pinned; start over from where the file is now.
                        finalPath = ProcessorPathIntegrity.GetFinalPath(file.ContentHandle);
                    }

                    this.ReleaseDirectoryHandles();

                    if (attempt + 1 >= MaximumPinAttempts)
                    {
                        throw new DscProcessorPathChangedException();
                    }
                }

                this.boundPath = path;
                this.boundIsAlias = isAlias;
                this.pinnedFile = file;
                this.launchPath = finalPath;
            }
            catch
            {
                this.ReleaseDirectoryHandles();
                file.Dispose();
                throw;
            }
        }

        private void PinAncestors(string finalPath)
        {
            // Stop below the root; a volume root cannot be renamed or deleted, and holding a handle
            // to it would interfere with operations such as dismounting the volume.
            for (string? directory = Path.GetDirectoryName(finalPath);
                 directory != null && Path.GetDirectoryName(directory) != null;
                 directory = Path.GetDirectoryName(directory))
            {
                this.directoryHandles.Add(ProcessorPathIntegrity.PinDirectory(directory));
            }
        }

        private void ReleaseDirectoryHandles()
        {
            foreach (SafeFileHandle directoryHandle in this.directoryHandles)
            {
                directoryHandle.Dispose();
            }

            this.directoryHandles.Clear();
        }
    }
}
