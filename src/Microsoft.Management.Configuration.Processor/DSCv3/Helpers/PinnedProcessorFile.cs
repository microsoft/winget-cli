// -----------------------------------------------------------------------------
// <copyright file="PinnedProcessorFile.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using Microsoft.Win32.SafeHandles;

    /// <summary>
    /// The handles that pin the processor file.
    /// <para>
    /// <see cref="NameHandle"/> is opened without following reparse points, so it pins whatever
    /// object sits at the given path; nothing else can delete it, rename it, move another file over
    /// it, or - if it is a link - repoint it, because all of those require write or delete access.
    /// </para>
    /// <para>
    /// <see cref="ContentHandle"/> pins the file that was actually hashed. It differs from
    /// <see cref="NameHandle"/> only when the path is a link, in which case both ends of the link
    /// are pinned.
    /// </para>
    /// </summary>
    internal sealed class PinnedProcessorFile : IDisposable
    {
        private readonly SafeFileHandle nameHandle;
        private readonly SafeFileHandle? targetHandle;
        private bool disposed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="PinnedProcessorFile"/> class.
        /// </summary>
        /// <param name="nameHandle">The handle that pins the object at the path itself.</param>
        /// <param name="targetHandle">The handle that pins the link target, or null if the path is not a link.</param>
        public PinnedProcessorFile(SafeFileHandle nameHandle, SafeFileHandle? targetHandle)
        {
            this.nameHandle = nameHandle;
            this.targetHandle = targetHandle;
        }

        /// <summary>
        /// Gets the handle pinning the object at the path itself.
        /// </summary>
        public SafeFileHandle NameHandle
        {
            get { return this.nameHandle; }
        }

        /// <summary>
        /// Gets the handle pinning the file whose content was hashed.
        /// </summary>
        public SafeFileHandle ContentHandle
        {
            get { return this.targetHandle ?? this.nameHandle; }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            if (!this.disposed)
            {
                this.targetHandle?.Dispose();
                this.nameHandle.Dispose();
                this.disposed = true;
            }
        }
    }
}
