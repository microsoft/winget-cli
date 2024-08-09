// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilities.cs" company="Microsoft Corporation">
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
    /// Download wrapper.
    /// </summary>
    public sealed class WinGetUtilities : IWinGetUtilities
    {
        /// <inheritdoc/>
        public string Download(string url, string filePath, bool enableLogging = false, string logFilePath = null)
        {
            IWinGetLogging log = null;
            if (enableLogging)
            {
                if (string.IsNullOrWhiteSpace(logFilePath))
                {
                    throw new ArgumentNullException($"Download logging is enabled and log file path `{logFilePath}` is not provided.");
                }

                log = new WinGetFactory().LoggingInit(logFilePath);
            }

            try
            {
                byte[] sha256Hash = new byte[32];
                WinGetDownload(url, filePath, sha256Hash, (uint)sha256Hash.Length);
                return BitConverter.ToString(sha256Hash).Replace("-", string.Empty);
            }
            catch (Exception e)
            {
                throw new WinGetDownloadException(e);
            }
            finally
            {
                log?.Dispose();
            }
        }

        /// <inheritdoc/>
        public int CompareVersions(string version1, string version2)
        {
            WinGetCompareVersions(version1, version2, out int result);
            return result;
        }

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetDownload(string url, string filePath, [MarshalAs(UnmanagedType.LPArray)] byte[] sha26Hash, uint sha256HashLength);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetCompareVersions(string version1, string version2, [MarshalAs(UnmanagedType.U4)] out int comparisonResult);
    }
}
