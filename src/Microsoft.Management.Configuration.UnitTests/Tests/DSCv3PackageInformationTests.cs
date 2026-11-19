// -----------------------------------------------------------------------------
// <copyright file="DSCv3PackageInformationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Management.Deployment;
    using Xunit;

    /// <summary>
    /// Tests for DSC package discovery and registration repair.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DSCv3PackageInformationTests
    {
        /// <summary>
        /// Healthy packages are not re-registered.
        /// </summary>
        [Fact]
        public void EnsureHealthy_HealthyPackage_DoesNotRegister()
        {
            PackageInformation.EnsureHealthy(
                "TestPackage",
                () => true,
                () => throw new InvalidOperationException("A healthy package must not be re-registered."),
                null);
        }

        /// <summary>
        /// An unhealthy package is re-registered and its refreshed status is checked.
        /// </summary>
        [Fact]
        public void EnsureHealthy_UnhealthyPackage_RegistersAndRechecks()
        {
            bool healthy = false;
            int statusChecks = 0;
            int registrations = 0;

            PackageInformation.EnsureHealthy(
                "TestPackage",
                () =>
                {
                    ++statusChecks;
                    return healthy;
                },
                () =>
                {
                    ++registrations;
                    healthy = true;
                },
                null);

            Assert.Equal(1, registrations);
            Assert.Equal(2, statusChecks);
        }

        /// <summary>
        /// A registration that leaves the package unhealthy fails without retrying indefinitely.
        /// </summary>
        [Fact]
        public void EnsureHealthy_RepairDoesNotRestoreHealth_Throws()
        {
            int registrations = 0;

            var exception = Assert.Throws<InvalidOperationException>(() => PackageInformation.EnsureHealthy(
                "TestPackage",
                () => false,
                () => ++registrations,
                null));

            Assert.Contains("TestPackage", exception.Message);
            Assert.Equal(1, registrations);
        }

        /// <summary>
        /// Registration failures propagate to the caller without further attempts.
        /// </summary>
        [Fact]
        public void EnsureHealthy_RegistrationFails_PropagatesError()
        {
            var expected = new UnauthorizedAccessException("Registration denied.");
            int statusChecks = 0;

            var exception = Assert.Throws<UnauthorizedAccessException>(() => PackageInformation.EnsureHealthy(
                "TestPackage",
                () =>
                {
                    ++statusChecks;
                    return false;
                },
                () => throw expected,
                null));

            Assert.Same(expected, exception);
            Assert.Equal(1, statusChecks);
        }

        /// <summary>
        /// An arbitrary executable cannot opt out of ancestor pinning by using the DSC file name.
        /// </summary>
        [Fact]
        public void FindTrustedPackageRoot_CustomPath_IsNotTrusted()
        {
            using var directory = new TempDirectory();
            string path = Path.Combine(directory.FullDirectoryPath, "dsc.exe");
            File.WriteAllText(path, "not a packaged executable");

            Assert.Null(PackageInformation.FindTrustedPackageRoot(path));
        }

        /// <summary>
        /// Discovery returns the executable in the selected package, not its app execution alias.
        /// </summary>
        /// <param name="familyName">The DSC package family to discover.</param>
        [SkippableTheory]
        [InlineData("Microsoft.DesiredStateConfiguration_8wekyb3d8bbwe")]
        [InlineData("Microsoft.DesiredStateConfiguration-Preview_8wekyb3d8bbwe")]
        public void GetExecutablePath_InstalledPackage_ReturnsPackagePath(string familyName)
        {
            PackageManager packageManager = new PackageManager();
            var package = packageManager.FindPackagesForUserWithPackageTypes(null, familyName, PackageTypes.Main)
                .OrderByDescending(package => new Version(package.Id.Version.Major, package.Id.Version.Minor, package.Id.Version.Build, package.Id.Version.Revision))
                .FirstOrDefault() ?? throw new SkipException("The DSC package is not installed.");

            Skip.IfNot(package.Status.VerifyIsOK(), "This test must not repair an installed package.");

            var information = new PackageInformation(familyName);
            string? executablePath = information.GetExecutablePath(null);

            Assert.True(information.IsInstalled);
            Assert.Equal(Path.Combine(package.InstalledLocation.Path, "dsc.exe"), executablePath);
            Assert.True(File.Exists(executablePath));
            string hash = ProcessorPathIntegrity.ComputeHash(executablePath!, out bool isAlias);
            Assert.False(isAlias);

            using var binding = new ProcessorPathBinding();
            string launchPath = binding.EnsurePinned(executablePath!, hash, isAlias);
            using var process = new Process()
            {
                StartInfo = new ProcessStartInfo(launchPath, "--version")
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                },
            };

            process.Start();
            bool exited = process.WaitForExit(30000);
            if (!exited)
            {
                process.Kill();
                process.WaitForExit();
            }

            Assert.True(exited, "The packaged DSC executable did not exit within 30 seconds.");
            Assert.Equal(0, process.ExitCode);
        }
    }
}
