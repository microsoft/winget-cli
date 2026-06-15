// -----------------------------------------------------------------------------
// <copyright file="PSPackageVersionInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System.Linq;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PackageVersionInfo wrapper object for displaying to PowerShell.
    /// </summary>
    public sealed class PSPackageVersionInfo
    {
        private PackageVersionInfo packageVersionInfo;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSPackageVersionInfo"/> class.
        /// </summary>
        /// <param name="packageVersionInfo">PackageVersionInfo COM object.</param>
        internal PSPackageVersionInfo(PackageVersionInfo packageVersionInfo)
        {
            this.packageVersionInfo = packageVersionInfo;
        }

        /// <summary>
        /// Gets the name of the package version info.
        /// </summary>
        public string DisplayName
        {
            get
            {
                return this.packageVersionInfo.DisplayName;
            }
        }

        /// <summary>
        /// Gets the id of the package version info.
        /// </summary>
        public string Id
        {
            get
            {
                return this.packageVersionInfo.Id;
            }
        }

        /// <summary>
        /// Gets the publisher of the package version info.
        /// </summary>
        public string Publisher
        {
            get
            {
                return this.packageVersionInfo.Publisher;
            }
        }

        /// <summary>
        /// Gets the channel of the package version info.
        /// </summary>
        public string Channel
        {
            get
            {
                return this.packageVersionInfo.Channel;
            }
        }

        /// <summary>
        /// Gets the list of package family names of the package version info.
        /// </summary>
        public string[] PackageFamilyNames
        {
            get
            {
                return this.packageVersionInfo.PackageFamilyNames.ToArray();
            }
        }

        /// <summary>
        /// Gets the list of product codes of the package version info.
        /// </summary>
        public string[] ProductCodes
        {
            get
            {
                return this.packageVersionInfo.ProductCodes.ToArray();
            }
        }

        /// <summary>
        /// Compares the version string with the package version info and returns the CompareResult.
        /// </summary>
        /// <param name="version">Version string.</param>
        /// <returns>CompareResult string.</returns>
        public string CompareToVersion(string version)
        {
            return this.packageVersionInfo.CompareToVersion(version).ToString();
        }
    }
}
