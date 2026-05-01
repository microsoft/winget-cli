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
    /// Provides integrity verification for the DSC processor executable path.
    /// Handles both regular files and app execution alias reparse points.
    /// </summary>
    internal static class ProcessorPathIntegrity
    {
        private const uint GenericRead = 0x80000000;
        private const uint FileShareRead = 0x00000001;
        private const uint FileShareExecute = 0x00000004;
        private const uint OpenExisting = 3;
        private const uint FileAttributeNormal = 0x80;
        private const uint FileFlagOpenReparsePoint = 0x00200000;
        private const uint FileFlagBackupSemantics = 0x02000000;
        private const uint FsctlGetReparsePoint = 0x000900A8;
        private const int MaximumReparseDataBufferSize = 16 * 1024;

        /// <summary>
        /// Opens the processor path, verifies its hash matches the expected value, and returns
        /// an open handle for TOCTOU protection.
        /// </summary>
        /// <param name="path">The path to the DSC executable or app execution alias.</param>
        /// <param name="expectedHash">The expected SHA256 hash (hex string, case-insensitive).</param>
        /// <param name="isAlias">Whether the path is an app execution alias reparse point.</param>
        /// <returns>An open SafeFileHandle to the file; caller should hold this for the process lifetime.</returns>
        public static SafeFileHandle VerifyAndOpen(string path, string expectedHash, bool isAlias)
        {
            SafeFileHandle handle;
            byte[] hashBytes;

            if (isAlias)
            {
                handle = CreateFile(
                    path,
                    0,
                    FileShareRead | FileShareExecute,
                    IntPtr.Zero,
                    OpenExisting,
                    FileFlagOpenReparsePoint | FileFlagBackupSemantics,
                    IntPtr.Zero);

                if (handle.IsInvalid)
                {
                    throw new InvalidOperationException($"Failed to open processor path alias '{path}': Win32 error {Marshal.GetLastWin32Error()}");
                }

                byte[] reparseBuffer = new byte[MaximumReparseDataBufferSize];
                if (!DeviceIoControl(handle, FsctlGetReparsePoint, IntPtr.Zero, 0, reparseBuffer, (uint)reparseBuffer.Length, out uint bytesReturned, IntPtr.Zero))
                {
                    handle.Dispose();
                    throw new InvalidOperationException($"Failed to read reparse data for '{path}': Win32 error {Marshal.GetLastWin32Error()}");
                }

                hashBytes = SHA256.HashData(reparseBuffer.AsSpan(0, (int)bytesReturned));
            }
            else
            {
                handle = CreateFile(
                    path,
                    GenericRead,
                    FileShareRead | FileShareExecute,
                    IntPtr.Zero,
                    OpenExisting,
                    FileAttributeNormal,
                    IntPtr.Zero);

                if (handle.IsInvalid)
                {
                    throw new InvalidOperationException($"Failed to open processor path '{path}': Win32 error {Marshal.GetLastWin32Error()}");
                }

                hashBytes = ComputeSHA256FromHandle(handle);
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
        /// </summary>
        /// <param name="path">The path to hash.</param>
        /// <param name="isAlias">Receives true if the path is an app execution alias reparse point.</param>
        /// <returns>The SHA256 hash as a lowercase hex string.</returns>
        public static string ComputeHash(string path, out bool isAlias)
        {
            // Attempt to open as a regular file first.
            SafeFileHandle regularHandle = CreateFile(
                path,
                GenericRead,
                FileShareRead | FileShareExecute,
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
                0,
                FileShareRead | FileShareExecute,
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
                byte[] reparseBuffer = new byte[MaximumReparseDataBufferSize];
                if (!DeviceIoControl(aliasHandle, FsctlGetReparsePoint, IntPtr.Zero, 0, reparseBuffer, (uint)reparseBuffer.Length, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new InvalidOperationException($"Failed to read reparse data for '{path}': Win32 error {Marshal.GetLastWin32Error()}");
                }

                isAlias = true;
                return Convert.ToHexString(SHA256.HashData(reparseBuffer.AsSpan(0, (int)bytesReturned))).ToLowerInvariant();
            }
        }

        private static byte[] ComputeSHA256FromHandle(SafeFileHandle handle)
        {
            using var incrHash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);

            byte[] buffer = new byte[1024 * 1024];
            while (true)
            {
                if (!ReadFile(handle, buffer, (uint)buffer.Length, out uint bytesRead, IntPtr.Zero) || bytesRead == 0)
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
