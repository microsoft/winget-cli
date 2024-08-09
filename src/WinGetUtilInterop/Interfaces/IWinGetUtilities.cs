// -----------------------------------------------------------------------------
// <copyright file="IWinGetUtilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Interfaces
{
    /// <summary>
    /// Winget Util utilities interface.
    /// </summary>
    public interface IWinGetUtilities
    {
        /// <summary>
        /// Download a file from URL.
        /// </summary>
        /// <param name="url">Installer url.</param>
        /// <param name="filePath">Destination file path.</param>
        /// <param name="enableLogging">Enable logging.</param>
        /// <param name="logFilePath">Log file path.</param>
        /// <returns>SHA256 of the installer.</returns>
        string Download(string url, string filePath, bool enableLogging = false, string logFilePath = null);

        /// <summary>
        /// Compare two version.
        /// </summary>
        /// <param name="version1">Version 1.</param>
        /// <param name="version2">Version 2.</param>
        /// <returns>Integer value indicating the comparison result.</returns>
        int CompareVersions(string version1, string version2);
    }
}
