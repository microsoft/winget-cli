// -----------------------------------------------------------------------------
// <copyright file="PackageInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.IO;
    using Windows.Management.Deployment;

    /// <summary>
    /// Contains information about a package.
    /// </summary>
    internal class PackageInformation
    {
        private const string DscExecutableFileName = "dsc.exe";

        /// <summary>
        /// Initializes a new instance of the <see cref="PackageInformation"/> class.
        /// </summary>
        /// <param name="familyName">The package family name.</param>
        public PackageInformation(string familyName)
        {
            PackageManager packageManager = new PackageManager();

            var packages = packageManager.FindPackagesForUserWithPackageTypes(null, familyName, PackageTypes.Main);

            if (packages != null)
            {
                foreach (var package in packages)
                {
                    var packageVersion = package.Id.Version;
                    Version version = new Version(packageVersion.Major, packageVersion.Minor, packageVersion.Build, packageVersion.Revision);

                    if (this.Version == null || version > this.Version)
                    {
                        this.Version = version;
                    }
                }
            }

            string localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            string aliasPath = Path.Combine(localAppData, "Microsoft\\WindowsApps", familyName, DscExecutableFileName);

            if (Path.Exists(aliasPath))
            {
                this.AliasPath = aliasPath;
            }

            if (this.AliasPath != null && this.Version != null)
            {
                this.IsInstalled = true;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the package is installed or not.
        /// </summary>
        public bool IsInstalled { get; private set; }

        /// <summary>
        /// Gets the path to the dsc.exe alias.
        /// </summary>
        public string? AliasPath { get; private set; }

        /// <summary>
        /// Gets the version of the package.
        /// </summary>
        public Version? Version { get; private set; }
    }
}
