// -----------------------------------------------------------------------------
// <copyright file="UpgradeCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test upgrade command.
    /// </summary>
    public class UpgradeCommand : BaseCommand
    {
        private static readonly string DenyUpgradePackage = "AppInstallerTest.TestUpgradeDeny";

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            // Due to its properties, this being present is problematic.
            TestCommon.RunAICLICommand("uninstall", DenyUpgradePackage);
        }

        /// <summary>
        /// Test upgrade portable package.
        /// </summary>
        [Test]
        public void UpgradePortable()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package.
        /// </summary>
        [Test]
        public void UpgradePortableNonAsciiPath()
        {
            string installDir = Path.Combine(TestCommon.GetRandomTestDir(), "Tést");
            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestPortableExe -v 1.0.0.0 -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0 -l {installDir}");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(installDir, commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package with arp mismatch.
        /// </summary>
        [Test]
        public void UpgradePortableARPMismatch()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string productCode = packageId + "_" + Constants.TestSourceIdentifier;

            var installResult = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

            // Modify packageId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");

            // Reset and perform uninstall cleanup
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, packageId);
            TestCommon.RunAICLICommand("uninstall", $"--product-code {productCode}");

            Assert.That(upgradeResult.ExitCode, Is.Not.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(upgradeResult.StdOut, Does.Contain("Portable package from a different source already exists"));
        }

        /// <summary>
        /// Test upgrade portable package force override.
        /// </summary>
        [Test]
        public void UpgradePortableForcedOverride()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var installResult = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

            // Modify packageId and sourceId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetSourceIdentifier, "testSourceId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0 --force");
            Assert.That(upgradeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(upgradeResult.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package uninstall previous version.
        /// </summary>
        [Test]
        public void UpgradePortableUninstallPrevious()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 3.0.0.0");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade with deny behavior.
        /// </summary>
        [Test]
        public void UpgradeBehaviorDeny()
        {
            string packageId = DenyUpgradePackage;

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.APPINSTALLER_CLI_ERROR_INSTALL_UPGRADE_NOT_SUPPORTED));
            Assert.That(result2.StdOut, Does.Contain("package cannot be upgraded using WinGet"));
        }

        /// <summary>
        /// Test upgrade portable package machine scope.
        /// </summary>
        [Test]
        public void UpgradePortableMachineScope()
        {
            string installDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, installDir);

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0 --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, string.Empty);
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.Machine);
        }

        /// <summary>
        /// Test upgrade zip portable package.
        /// </summary>
        [Test]
        public void UpgradeZip_Portable()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestZipInstallerWithPortable";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = "TestPortable.exe";
            fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithPortable -v 1.0.0.0");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.User);
        }

        /// <summary>
        /// Test upgrade zip portable package re-evaluates stale InstallDirectoryAddedToPath state.
        /// </summary>
        [Test]
        public void UpgradeZip_Portable_ReevaluatesInstallDirectoryAddedToPath()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string linksDirectory = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string packageId = "AppInstallerTest.TestZipInstallerPortablePathFallbackToSymlink";
            string packageDir = Path.Combine(installDir, packageId + "_" + Constants.TestSourceIdentifier);
            string symlinkPath = Path.Combine(linksDirectory, "TestPortableTransitionPathToSymlink.exe");

            var installResult = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.That(upgradeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(symlinkPath, Does.Exist);
            Assert.That(TestCommon.PathContainsValue(linksDirectory));
            Assert.That(TestCommon.PathContainsValue(packageDir), Is.False);
        }

        /// <summary>
        /// Test upgrade zip portable package with binaries dependent on PATH cleans stale Links PATH entry.
        /// </summary>
        [Test]
        public void UpgradeZip_ArchivePortableWithBinariesDependentOnPath_CleansLinksPath()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId = "AppInstallerTest.TestZipInstallerPortableSymlinkToPathFallback";
            string packageDir = Path.Combine(installDir, packageId + "_" + Constants.TestSourceIdentifier);
            string linksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);

            var installResult = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            Assert.That(TestCommon.PathContainsValue(linksDir));

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.That(upgradeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(TestCommon.PathContainsValue(packageDir));

            if (!Directory.Exists(linksDir) || Directory.GetFileSystemEntries(linksDir).Length == 0)
            {
                Assert.That(TestCommon.PathContainsValue(linksDir), Is.False);
            }
        }

        /// <summary>
        /// Test upgrade when a new dependency is added that is not installed.
        /// </summary>
        [Test]
        public void UpgradeAddsDependency()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestUpgradeAddsDependency -v 1.0 --verbose");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("upgrade", $"AppInstallerTest.TestUpgradeAddsDependency");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
        }
    }
}
