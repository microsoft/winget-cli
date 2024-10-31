// -----------------------------------------------------------------------------
// <copyright file="WinGetVersion.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// WinGetVersion. Parse the string version returned by winget --version to allow comparisons.
    /// </summary>
    internal class WinGetVersion
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetVersion"/> class.
        /// </summary>
        /// <param name="version">String Version.</param>
        public WinGetVersion(string version)
        {
            if (string.IsNullOrEmpty(version))
            {
                throw new ArgumentNullException();
            }

            string toParseVersion = version;

            // WinGet version starts with v
            if (toParseVersion[0] == 'v')
            {
                this.TagVersion = version;
                toParseVersion = toParseVersion.Substring(1);
            }
            else
            {
                // WinGet version always start with v.
                this.TagVersion = 'v' + version;
            }

            // WinGet version might end with -preview
            if (toParseVersion.EndsWith("-preview"))
            {
                this.IsPrerelease = true;
                toParseVersion = toParseVersion.Substring(0, toParseVersion.IndexOf('-'));
            }

            this.Version = Version.Parse(toParseVersion);
        }

        /// <summary>
        /// Gets the version as it appears as a tag.
        /// </summary>
        public string TagVersion { get; }

        /// <summary>
        /// Gets the version.
        /// </summary>
        public System.Version Version { get; }

        /// <summary>
        /// Gets a value indicating whether is this version is a prerelease.
        /// </summary>
        public bool IsPrerelease { get; }

        /// <summary>
        /// Gets the version of the installed winget.
        /// </summary>
        /// <param name="pwshCmdlet">PowerShell cmdlet.</param>
        /// <returns>The WinGetVersion.</returns>
        public static WinGetVersion InstalledWinGetVersion(PowerShellCmdlet pwshCmdlet)
        {
            var wingetCliWrapper = new WingetCLIWrapper();
            var result = wingetCliWrapper.RunCommand(pwshCmdlet, "--version");
            return new WinGetVersion(result.StdOut.Replace(Environment.NewLine, string.Empty));
        }

        /// <summary>
        /// Version.CompareTo taking into account prerelease.
        /// From semver: Pre-release versions have a lower precedence than the associated normal version.
        /// </summary>
        /// <param name="otherVersion">Other winget version.</param>
        /// <returns>
        /// A signed integer that indicates the relative values of the two objects. Less than 0
        /// means this version is before other version. 0 means they are equal. Greater than 0
        /// means this version is greater.
        /// </returns>
        public int CompareTo(WinGetVersion otherVersion)
        {
            if (this.IsPrerelease && !otherVersion.IsPrerelease)
            {
                return -1;
            }

            if (!this.IsPrerelease && otherVersion.IsPrerelease)
            {
                return 1;
            }

            return this.Version.CompareTo(otherVersion.Version);
        }

        /// <summary>
        /// Deployment doesn't care about semver or prerelease builds.
        /// </summary>
        /// <param name="otherVersion">Other version.</param>
        /// <returns>
        /// A signed integer that indicates the relative values of the two objects. Less than 0
        /// means this version is before other version. 0 means they are equal. Greater than 0
        /// means this version is greater.
        /// </returns>
        public int CompareAsDeployment(WinGetVersion otherVersion)
        {
            return this.Version.CompareTo(otherVersion.Version);
        }
    }
}
