// -----------------------------------------------------------------------------
// <copyright file="ProcessorPathIntegrity.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Buffers.Binary;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Win32.SafeHandles;

    /// <summary>
    /// Provides integrity verification and pinning for the DSC processor executable path.
    /// Handles both regular files and app execution alias reparse points.
    /// </summary>
    internal static class ProcessorPathIntegrity
    {
        // FILE_READ_DATA, which is also FILE_LIST_DIRECTORY for a directory. A handle opened
        // without any data access right does not participate in share access checks at all, so
        // requesting this is what makes the share mode below actually deny anything.
        private const uint FileReadData = 0x00000001;

        private const uint FileShareRead = 0x00000001;
        private const uint FileShareWrite = 0x00000002;
        private const uint FileShareDelete = 0x00000004;

        private const uint OpenExisting = 3;
        private const uint FileAttributeNormal = 0x80;
        private const uint FileAttributeReparsePoint = 0x00000400;
        private const uint IoReparseTagAppExecLink = 0x8000001B;
        private const uint FileFlagOpenReparsePoint = 0x00200000;
        private const uint FileFlagBackupSemantics = 0x02000000;
        private const uint FsctlGetReparsePoint = 0x000900A8;
        private const uint FileNameNormalized = 0x0;
        private const uint VolumeNameDos = 0x0;
        private const int MaximumReparseDataBufferSize = 16 * 1024;

        /// <summary>
        /// Opens the processor path with a share mode that prevents the file from being modified,
        /// replaced, renamed or deleted, then verifies that its hash matches the expected value.
        /// <para>
        /// The path is opened without following reparse points, so the returned pin covers the
        /// object at the path itself. If that object is a link, the link target is pinned as well;
        /// holding both means the link cannot be retargeted (that requires write access) and the
        /// target cannot be replaced, so the path cannot be made to resolve to anything else.
        /// </para>
        /// </summary>
        /// <param name="path">The path to the DSC executable or app execution alias.</param>
        /// <param name="expectedHash">The expected SHA256 hash (hex string, case-insensitive).</param>
        /// <param name="isAlias">Whether the path is an app execution alias reparse point.</param>
        /// <returns>The pinned file; the caller must hold this for as long as the path is used.</returns>
        public static PinnedProcessorFile VerifyAndOpen(string path, string expectedHash, bool isAlias)
        {
            SafeFileHandle nameHandle = OpenNoFollow(path, FileReadData, FileShareRead);
            SafeFileHandle? targetHandle = null;
            byte[] hashBytes;

            try
            {
                // The hash is always computed through a pinned handle, so the bytes that are
                // hashed are by definition the bytes of the file object that remains pinned.
                if (IsReparsePoint(nameHandle, path))
                {
                    byte[] reparseData = ReadReparseData(nameHandle, path);

                    if (GetReparseTag(reparseData) == IoReparseTagAppExecLink)
                    {
                        if (!isAlias)
                        {
                            throw new DscProcessorPathChangedException($"The processor path '{path}' is an app execution alias, but was not expected to be one.");
                        }

                        hashBytes = SHA256.HashData(reparseData);
                    }
                    else
                    {
                        if (isAlias)
                        {
                            throw new DscProcessorPathChangedException($"The processor path '{path}' was expected to be an app execution alias, but is a different kind of reparse point.");
                        }

                        // A link; pin the target as well so that the path cannot be made to
                        // resolve to a different file while it is in use.
                        targetHandle = Open(path, FileReadData, FileShareRead, FileAttributeNormal);
                        hashBytes = ComputeSHA256FromHandle(targetHandle);
                    }
                }
                else
                {
                    if (isAlias)
                    {
                        throw new DscProcessorPathChangedException($"The processor path '{path}' was expected to be an app execution alias, but is a regular file.");
                    }

                    hashBytes = ComputeSHA256FromHandle(nameHandle);
                }

                string computedHash = Convert.ToHexString(hashBytes).ToLowerInvariant();
                if (!string.Equals(computedHash, expectedHash, StringComparison.OrdinalIgnoreCase))
                {
                    throw new DscProcessorHashMismatchException();
                }
            }
            catch
            {
                targetHandle?.Dispose();
                nameHandle.Dispose();
                throw;
            }

            return new PinnedProcessorFile(nameHandle, targetHandle);
        }

        /// <summary>
        /// Computes the SHA256 hash of a path, auto-detecting whether it is an app execution alias.
        /// This is a measurement only; it does not pin the file.
        /// </summary>
        /// <param name="path">The path to hash.</param>
        /// <param name="isAlias">Receives true if the path is an app execution alias reparse point.</param>
        /// <returns>The SHA256 hash as a lowercase hex string.</returns>
        public static string ComputeHash(string path, out bool isAlias)
        {
            // Sharing is permissive here because this is only a measurement of the current
            // content; pinning happens in VerifyAndOpen.
            const uint ShareAll = FileShareRead | FileShareWrite | FileShareDelete;

            using SafeFileHandle nameHandle = OpenNoFollow(path, FileReadData, ShareAll);

            if (IsReparsePoint(nameHandle, path))
            {
                byte[] reparseData = ReadReparseData(nameHandle, path);

                if (GetReparseTag(reparseData) == IoReparseTagAppExecLink)
                {
                    isAlias = true;
                    return Convert.ToHexString(SHA256.HashData(reparseData)).ToLowerInvariant();
                }

                isAlias = false;
                using SafeFileHandle targetHandle = Open(path, FileReadData, ShareAll, FileAttributeNormal);
                return Convert.ToHexString(ComputeSHA256FromHandle(targetHandle)).ToLowerInvariant();
            }

            isAlias = false;
            return Convert.ToHexString(ComputeSHA256FromHandle(nameHandle)).ToLowerInvariant();
        }

        /// <summary>
        /// Gets the normalized, reparse point free path of the file that a handle refers to.
        /// </summary>
        /// <param name="handle">The handle to get the path of.</param>
        /// <returns>The final path, without the extended length prefix.</returns>
        public static string GetFinalPath(SafeFileHandle handle)
        {
            uint length = GetFinalPathNameByHandle(handle, null, 0, FileNameNormalized | VolumeNameDos);
            if (length == 0)
            {
                throw new InvalidOperationException($"Failed to get the final path of the processor: Win32 error {Marshal.GetLastWin32Error()}");
            }

            char[] buffer = new char[length + 1];
            uint copied = GetFinalPathNameByHandle(handle, buffer, (uint)buffer.Length, FileNameNormalized | VolumeNameDos);
            if (copied == 0 || copied >= buffer.Length)
            {
                throw new InvalidOperationException($"Failed to get the final path of the processor: Win32 error {Marshal.GetLastWin32Error()}");
            }

            return RemoveExtendedLengthPrefix(new string(buffer, 0, (int)copied));
        }

        /// <summary>
        /// Opens a directory with a share mode that denies delete access to every other opener,
        /// which prevents the directory from being renamed or deleted while the handle is held.
        /// Creating and deleting files within the directory remains possible.
        /// <para>
        /// The directory is opened without following reparse points and is rejected if it is one.
        /// Callers pin the components of an already normalized path, which contains no reparse
        /// points, so finding one means that a component was replaced while the path was being
        /// pinned. Once pinned, a directory cannot become a reparse point: that requires the
        /// directory to be empty, and it always contains the pinned component below it.
        /// </para>
        /// </summary>
        /// <param name="path">The directory to pin.</param>
        /// <returns>An open handle to the directory.</returns>
        public static SafeFileHandle PinDirectory(string path)
        {
            SafeFileHandle handle = OpenNoFollow(path, FileReadData, FileShareRead | FileShareWrite);

            try
            {
                if (IsReparsePoint(handle, path))
                {
                    throw new DscProcessorPathChangedException($"The processor path directory '{path}' was replaced with a reparse point.");
                }
            }
            catch
            {
                handle.Dispose();
                throw;
            }

            return handle;
        }

        private static SafeFileHandle Open(string path, uint access, uint shareMode, uint flagsAndAttributes)
        {
            // Data access is requested in every case; a handle opened without any data access
            // right does not participate in share access checks and would pin nothing.
            SafeFileHandle handle = CreateFile(
                path,
                access,
                shareMode,
                IntPtr.Zero,
                OpenExisting,
                flagsAndAttributes,
                IntPtr.Zero);

            if (handle.IsInvalid)
            {
                throw new InvalidOperationException($"Failed to open processor path '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return handle;
        }

        private static SafeFileHandle OpenNoFollow(string path, uint access, uint shareMode)
        {
            // Backup semantics allows the open to succeed if the path names a directory; that is
            // then rejected by the hashing below rather than surfacing as an opaque Win32 error.
            return Open(path, access, shareMode, FileFlagOpenReparsePoint | FileFlagBackupSemantics);
        }

        private static bool IsReparsePoint(SafeFileHandle handle, string path)
        {
            if (!GetFileInformationByHandle(handle, out ByHandleFileInformation information))
            {
                throw new InvalidOperationException($"Failed to get file information for '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return (information.FileAttributes & FileAttributeReparsePoint) != 0;
        }

        private static uint GetReparseTag(byte[] reparseData)
        {
            if (reparseData.Length < sizeof(uint))
            {
                throw new DscProcessorPathChangedException("The processor path reparse data is too small to contain a tag.");
            }

            return BinaryPrimitives.ReadUInt32LittleEndian(reparseData);
        }

        private static byte[] ReadReparseData(SafeFileHandle handle, string path)
        {
            byte[] reparseBuffer = new byte[MaximumReparseDataBufferSize];
            if (!DeviceIoControl(handle, FsctlGetReparsePoint, IntPtr.Zero, 0, reparseBuffer, (uint)reparseBuffer.Length, out uint bytesReturned, IntPtr.Zero))
            {
                throw new InvalidOperationException($"Failed to read reparse data for '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return reparseBuffer[.. (int)bytesReturned];
        }

        private static string RemoveExtendedLengthPrefix(string path)
        {
            const string UncPrefix = @"\\?\UNC\";
            const string Prefix = @"\\?\";

            if (path.StartsWith(UncPrefix, StringComparison.Ordinal))
            {
                return string.Concat(@"\\", path.AsSpan(UncPrefix.Length));
            }

            if (path.StartsWith(Prefix, StringComparison.Ordinal))
            {
                return path.Substring(Prefix.Length);
            }

            return path;
        }

        private static byte[] ComputeSHA256FromHandle(SafeFileHandle handle)
        {
            using var incrHash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);

            byte[] buffer = new byte[1024 * 1024];
            while (true)
            {
                if (!ReadFile(handle, buffer, (uint)buffer.Length, out uint bytesRead, IntPtr.Zero))
                {
                    throw new InvalidOperationException($"Failed to read the processor file: Win32 error {Marshal.GetLastWin32Error()}");
                }

                if (bytesRead == 0)
                {
                    break;
                }

                incrHash.AppendData(buffer, 0, (int)bytesRead);
            }

            return incrHash.GetHashAndReset();
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern SafeFileHandle CreateFile(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "Marking the array as nullable.")]
        private static extern uint GetFinalPathNameByHandle(
            SafeFileHandle hFile,
            char[]? lpszFilePath,
            uint cchFilePath,
            uint dwFlags);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool DeviceIoControl(
            SafeFileHandle hDevice,
            uint dwIoControlCode,
            IntPtr lpInBuffer,
            uint nInBufferSize,
            byte[] lpOutBuffer,
            uint nOutBufferSize,
            out uint lpBytesReturned,
            IntPtr lpOverlapped);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetFileInformationByHandle(
            SafeFileHandle hFile,
            out ByHandleFileInformation lpFileInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool ReadFile(
            SafeFileHandle hFile,
            byte[] lpBuffer,
            uint nNumberOfBytesToRead,
            out uint lpNumberOfBytesRead,
            IntPtr lpOverlapped);

        [StructLayout(LayoutKind.Sequential)]
        private struct ByHandleFileInformation
        {
            public uint FileAttributes;
            public long CreationTime;
            public long LastAccessTime;
            public long LastWriteTime;
            public uint VolumeSerialNumber;
            public uint FileSizeHigh;
            public uint FileSizeLow;
            public uint NumberOfLinks;
            public uint FileIndexHigh;
            public uint FileIndexLow;
        }
    }
}
