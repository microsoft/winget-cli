// -----------------------------------------------------------------------------
// <copyright file="ProcessorPathIntegrity.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
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
        private const uint GenericRead = 0x80000000;
        private const uint FileReadAttributes = 0x00000080;

        // FILE_READ_DATA, which is also FILE_LIST_DIRECTORY for a directory. A handle opened
        // without any data access right does not participate in share access checks at all, so
        // requesting this is what makes the share mode below actually deny anything.
        private const uint FileReadData = 0x00000001;

        private const uint FileShareRead = 0x00000001;
        private const uint FileShareWrite = 0x00000002;
        private const uint FileShareDelete = 0x00000004;

        private const uint OpenExisting = 3;
        private const uint FileAttributeNormal = 0x80;
        private const uint FileFlagOpenReparsePoint = 0x00200000;
        private const uint FileFlagBackupSemantics = 0x02000000;
        private const uint FsctlGetReparsePoint = 0x000900A8;
        private const uint FileNameNormalized = 0x0;
        private const uint VolumeNameDos = 0x0;
        private const int MaximumReparseDataBufferSize = 16 * 1024;

        /// <summary>
        /// Opens the processor path with a share mode that prevents the file from being modified,
        /// replaced, renamed or deleted, then verifies that its hash matches the expected value.
        /// </summary>
        /// <param name="path">The path to the DSC executable or app execution alias.</param>
        /// <param name="expectedHash">The expected SHA256 hash (hex string, case-insensitive).</param>
        /// <param name="isAlias">Whether the path is an app execution alias reparse point.</param>
        /// <returns>An open SafeFileHandle to the file; the caller must hold this for as long as the path is used.</returns>
        public static SafeFileHandle VerifyAndOpen(string path, string expectedHash, bool isAlias)
        {
            SafeFileHandle handle = OpenForPin(path, isAlias);
            byte[] hashBytes;

            try
            {
                // The hash is always computed through the pinned handle, so the bytes that are
                // hashed are by definition the bytes of the file object that remains pinned.
                hashBytes = isAlias ? SHA256.HashData(ReadReparseData(handle, path)) : ComputeSHA256FromHandle(handle);
            }
            catch
            {
                handle.Dispose();
                throw;
            }

            string computedHash = Convert.ToHexString(hashBytes).ToLowerInvariant();
            if (!string.Equals(computedHash, expectedHash, StringComparison.OrdinalIgnoreCase))
            {
                handle.Dispose();
                throw new DscProcessorHashMismatchException();
            }

            return handle;
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
            // Attempt to open as a regular file first. Sharing is permissive here because this is
            // only a measurement of the current content; pinning happens in VerifyAndOpen.
            SafeFileHandle regularHandle = CreateFile(
                path,
                GenericRead,
                FileShareRead | FileShareWrite | FileShareDelete,
                IntPtr.Zero,
                OpenExisting,
                FileAttributeNormal,
                IntPtr.Zero);

            if (!regularHandle.IsInvalid)
            {
                isAlias = false;
                using (regularHandle)
                {
                    return Convert.ToHexString(ComputeSHA256FromHandle(regularHandle)).ToLowerInvariant();
                }
            }

            // If the regular open fails, try as an app execution alias reparse point.
            SafeFileHandle aliasHandle = CreateFile(
                path,
                FileReadAttributes,
                FileShareRead | FileShareWrite | FileShareDelete,
                IntPtr.Zero,
                OpenExisting,
                FileFlagOpenReparsePoint | FileFlagBackupSemantics,
                IntPtr.Zero);

            if (aliasHandle.IsInvalid)
            {
                throw new InvalidOperationException($"Failed to open path '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            using (aliasHandle)
            {
                isAlias = true;
                return Convert.ToHexString(SHA256.HashData(ReadReparseData(aliasHandle, path))).ToLowerInvariant();
            }
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
        /// </summary>
        /// <param name="path">The directory to pin.</param>
        /// <returns>An open handle to the directory.</returns>
        public static SafeFileHandle PinDirectory(string path)
        {
            SafeFileHandle handle = CreateFile(
                path,
                FileReadData,
                FileShareRead | FileShareWrite,
                IntPtr.Zero,
                OpenExisting,
                FileFlagBackupSemantics,
                IntPtr.Zero);

            if (handle.IsInvalid)
            {
                throw new InvalidOperationException($"Failed to pin processor path directory '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return handle;
        }

        private static SafeFileHandle OpenForPin(string path, bool isAlias)
        {
            // Read data access is requested in both cases; without it the handle would not
            // participate in share access checks and would therefore not pin anything.
            SafeFileHandle handle = isAlias ?
                CreateFile(
                    path,
                    FileReadData,
                    FileShareRead,
                    IntPtr.Zero,
                    OpenExisting,
                    FileFlagOpenReparsePoint | FileFlagBackupSemantics,
                    IntPtr.Zero) :
                CreateFile(
                    path,
                    GenericRead,
                    FileShareRead,
                    IntPtr.Zero,
                    OpenExisting,
                    FileAttributeNormal,
                    IntPtr.Zero);

            if (handle.IsInvalid)
            {
                throw new InvalidOperationException($"Failed to open processor path '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return handle;
        }

        private static byte[] ReadReparseData(SafeFileHandle handle, string path)
        {
            byte[] reparseBuffer = new byte[MaximumReparseDataBufferSize];
            if (!DeviceIoControl(handle, FsctlGetReparsePoint, IntPtr.Zero, 0, reparseBuffer, (uint)reparseBuffer.Length, out uint bytesReturned, IntPtr.Zero))
            {
                throw new InvalidOperationException($"Failed to read reparse data for '{path}': Win32 error {Marshal.GetLastWin32Error()}");
            }

            return reparseBuffer[..(int)bytesReturned];
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
        private static extern bool ReadFile(
            SafeFileHandle hFile,
            byte[] lpBuffer,
            uint nNumberOfBytesToRead,
            out uint lpNumberOfBytesRead,
            IntPtr lpOverlapped);
    }
}
