// -----------------------------------------------------------------------------
// <copyright file="TempDirectory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.IO;

    /// <summary>
    /// Creates a temporary directory in the user's temporary directory.
    /// </summary>
    internal class TempDirectory : IDisposable
    {
        private bool disposed = false;
        private bool cleanup;

        /// <summary>
        /// Initializes a new instance of the <see cref="TempDirectory"/> class.
        /// </summary>
        /// <param name="directoryName">Optional directory name. If null, creates a random directory name.</param>
        /// <param name="deleteIfExists">Delete directory if already exists. Default true.</param>
        /// <param name="cleanup">Deletes directory at disposing time. Default true.</param>
        public TempDirectory(
            string? directoryName = null,
            bool deleteIfExists = true,
            bool cleanup = true)
        {
            var path = Path.GetTempPath();

            if (directoryName is null)
            {
                this.DirectoryName = Path.GetRandomFileName();
            }
            else
            {
                this.DirectoryName = directoryName;
            }

            this.FullDirectoryPath = Path.Combine(Path.GetTempPath(), this.DirectoryName);

            if (deleteIfExists && Directory.Exists(this.FullDirectoryPath))
            {
                Directory.Delete(this.FullDirectoryPath, true);
            }

            Directory.CreateDirectory(this.FullDirectoryPath);
            this.cleanup = cleanup;
        }

        /// <summary>
        /// Gets the directory name.
        /// </summary>
        public string DirectoryName { get; }

        /// <summary>
        /// Gets the full directory name.
        /// </summary>
        public string FullDirectoryPath { get; }

        /// <summary>
        /// IDisposable.Dispose .
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Copies all contents of a directory into this directory.
        /// </summary>
        /// <param name="sourceDir">Source directory.</param>
        public void CopyDirectory(string sourceDir)
        {
            this.CopyDirectory(sourceDir, this.FullDirectoryPath);
        }

        /// <summary>
        /// Protected disposed.
        /// </summary>
        /// <param name="disposing">Disposing.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (this.cleanup && Directory.Exists(this.FullDirectoryPath))
                {
                    Directory.Delete(this.FullDirectoryPath, true);
                }

                this.disposed = true;
            }
        }

        private void CopyDirectory(string sourceDir, string destinationDir)
        {
            var dir = new DirectoryInfo(sourceDir);

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(dir.FullName);
            }

            Directory.CreateDirectory(destinationDir);

            foreach (FileInfo file in dir.GetFiles())
            {
                file.CopyTo(Path.Combine(destinationDir, file.Name));
            }

            foreach (DirectoryInfo subDir in dir.GetDirectories())
            {
                this.CopyDirectory(subDir.FullName, Path.Combine(destinationDir, subDir.Name));
            }
        }
    }
}
