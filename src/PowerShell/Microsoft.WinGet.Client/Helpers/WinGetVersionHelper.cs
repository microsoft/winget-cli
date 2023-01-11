// -----------------------------------------------------------------------------
// <copyright file="WinGetVersionHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System;

    /// <summary>
    /// WinGetVersion helper.
    /// </summary>
    internal static class WinGetVersionHelper
    {
        /// <summary>
        /// Gets the version of the installed winget.
        /// </summary>
        public static string InstalledWinGetVersion
        {
            get
            {
                var wingetCliWrapper = new WingetCLIWrapper();
                var result = wingetCliWrapper.RunCommand("--version");
                return result.StdOut.Replace(Environment.NewLine, string.Empty);
            }
        }

        /// <summary>
        /// Converts a WinGet string format version to a Version object.
        /// </summary>
        /// <param name="version">Version string.</param>
        /// <returns>Version.</returns>
        public static Version ConvertWinGetVersion(string version)
        {
            if (string.IsNullOrEmpty(version))
            {
                throw new ArgumentNullException();
            }

            // WinGet version starts with v
            if (version[0] != 'v')
            {
                throw new ArgumentException();
            }

            version = version.Substring(1);

            // WinGet version might end with -preview
            if (version.EndsWith("-preview"))
            {
                version = version.Substring(0, version.IndexOf('-'));
            }

            return Version.Parse(version);
        }
    }
}
