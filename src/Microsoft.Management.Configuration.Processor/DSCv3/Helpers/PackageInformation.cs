// -----------------------------------------------------------------------------
// <copyright file="PackageInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.IO;
    using Windows.ApplicationModel;
    using Windows.Management.Deployment;

    /// <summary>
    /// Contains information about a package.
    /// </summary>
    internal class PackageInformation
    {
        /// <summary>
        /// The stable DSC package family name.
        /// </summary>
        public const string StableDscPackageFamilyName = "Microsoft.DesiredStateConfiguration_8wekyb3d8bbwe";

        /// <summary>
        /// The preview DSC package family name.
        /// </summary>
        public const string PreviewDscPackageFamilyName = "Microsoft.DesiredStateConfiguration-Preview_8wekyb3d8bbwe";

        private const string DscExecutableFileName = "dsc.exe";
        private Package? package;

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
                        this.package = package;
                    }
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether the package is installed or not.
        /// </summary>
        public bool IsInstalled => this.package != null;

        /// <summary>
        /// Gets the version of the package.
        /// </summary>
        public Version? Version { get; private set; }

        /// <summary>
        /// Gets the packaged DSC executable path, repairing an unhealthy registration first.
        /// </summary>
        /// <param name="diagnosticsSink">The diagnostics sink.</param>
        /// <returns>The executable path, or null if the package is not installed.</returns>
        public string? GetExecutablePath(IDiagnosticsSink? diagnosticsSink)
        {
            if (this.package == null)
            {
                return null;
            }

            Package selectedPackage = this.package;
            string packageFullName = selectedPackage.Id.FullName;
            EnsureHealthy(
                packageFullName,
                () => selectedPackage.Status.VerifyIsOK(),
                () =>
                {
                    PackageManager packageManager = new PackageManager();
                    DeploymentResult result = packageManager.RegisterPackageByFullNameAsync(
                        packageFullName, null, DeploymentOptions.None).AsTask().GetAwaiter().GetResult();

                    if (result.ExtendedErrorCode != null)
                    {
                        throw new InvalidOperationException($"Failed to re-register DSC package '{packageFullName}': {result.ErrorText}", result.ExtendedErrorCode);
                    }

                    selectedPackage = packageManager.FindPackageForUser(string.Empty, packageFullName)
                        ?? throw new InvalidOperationException($"DSC package '{packageFullName}' was not found after re-registration.");
                },
                diagnosticsSink);

            this.package = selectedPackage;
            string executablePath = Path.Combine(selectedPackage.InstalledLocation.Path, DscExecutableFileName);
            if (!File.Exists(executablePath))
            {
                throw new FileNotFoundException($"The DSC executable was not found in package '{packageFullName}'.", executablePath);
            }

            return executablePath;
        }

        /// <summary>
        /// Finds the immutable package directory containing a registered Store DSC executable.
        /// </summary>
        /// <param name="executablePath">The normalized executable path.</param>
        /// <returns>The package directory, or null if the path is not a trusted packaged executable.</returns>
        internal static string? FindTrustedPackageRoot(string executablePath)
        {
            if (!string.Equals(Path.GetFileName(executablePath), DscExecutableFileName, StringComparison.OrdinalIgnoreCase))
            {
                return null;
            }

            foreach (string familyName in new[] { StableDscPackageFamilyName, PreviewDscPackageFamilyName })
            {
                var information = new PackageInformation(familyName);
                Package? package = information.package;
                if (package == null || package.IsDevelopmentMode || package.SignatureKind != PackageSignatureKind.Store || !package.Status.VerifyIsOK())
                {
                    continue;
                }

                string packageRoot = package.InstalledLocation.Path;
                if (!string.Equals(executablePath, Path.Combine(packageRoot, DscExecutableFileName), StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                return packageRoot;
            }

            return null;
        }

        /// <summary>
        /// Re-registers an unhealthy package once and verifies that the repair succeeded.
        /// </summary>
        /// <param name="packageFullName">The package full name.</param>
        /// <param name="verifyIsOK">Checks the current package status.</param>
        /// <param name="register">Re-registers the package and refreshes its information.</param>
        /// <param name="diagnosticsSink">The diagnostics sink.</param>
        internal static void EnsureHealthy(string packageFullName, Func<bool> verifyIsOK, Action register, IDiagnosticsSink? diagnosticsSink)
        {
            if (verifyIsOK())
            {
                return;
            }

            diagnosticsSink?.OnDiagnostics(DiagnosticLevel.Warning, $"DSC package '{packageFullName}' is unhealthy; attempting re-registration.");
            register();

            if (!verifyIsOK())
            {
                throw new InvalidOperationException($"DSC package '{packageFullName}' is still unhealthy after re-registration.");
            }

            diagnosticsSink?.OnDiagnostics(DiagnosticLevel.Informational, $"DSC package '{packageFullName}' was successfully re-registered.");
        }
    }
}
