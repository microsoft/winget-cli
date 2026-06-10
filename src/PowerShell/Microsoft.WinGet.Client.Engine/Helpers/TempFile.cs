// -----------------------------------------------------------------------------
// <copyright file="TempFile.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.IO;

    /// <summary>
    /// Creates a temporary file in the user's temporary directory.
    /// </summary>
    internal class TempFile : IDisposable
    {
        private readonly bool cleanup;

        private bool disposed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="TempFile"/> class.
        /// </summary>
        /// <param name="fileName">Optional file name. If null, creates a random file name.</param>
        /// <param name="deleteIfExists">Delete file if already exists. Default true.</param>
        /// <param name="content">Optional content. If not null or empty, creates file and writes to it.</param>
        /// <param name="cleanup">Deletes file at disposing time. Default true.</param>
        public TempFile(
            string? fileName = null,
            bool deleteIfExists = true,
            string? content = null,
            bool cleanup = true)
        {
            if (fileName is null)
            {
                this.FileName = Path.GetRandomFileName();
                this.FullPath = Path.Combine(Path.GetTempPath(), this.FileName);
            }
            else
            {
                this.FileName = fileName;
                var randomDir = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                Directory.CreateDirectory(randomDir);
                this.FullPath = Path.Combine(randomDir, this.FileName);
            }

            if (deleteIfExists && File.Exists(this.FullPath))
            {
                File.Delete(this.FullPath);
            }

            if (!string.IsNullOrWhiteSpace(content))
            {
                this.CreateFile(content);
            }

            this.cleanup = cleanup;
        }

        /// <summary>
        /// Gets the file name.
        /// </summary>
        public string FileName { get; }

        /// <summary>
        /// Gets the full path.
        /// </summary>
        public string FullPath { get; }

        /// <summary>
        /// IDisposable.Dispose.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Creates the file.
        /// </summary>
        /// <param name="content">Content.</param>
        public void CreateFile(string? content = null)
        {
            if (content is null)
            {
                using var fs = File.Create(this.FullPath);
            }
            else
            {
                File.WriteAllText(this.FullPath, content);
            }
        }

        /// <summary>
        /// Protected disposed.
        /// </summary>
        /// <param name="disposing">Disposing.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (this.cleanup && File.Exists(this.FullPath))
                {
                    File.Delete(this.FullPath);
                }

                this.disposed = true;
            }
        }
    }
}
