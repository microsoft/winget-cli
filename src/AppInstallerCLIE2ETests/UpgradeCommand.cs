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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            // Modify packageId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");

            // Reset and perform uninstall cleanup
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, packageId);
            TestCommon.RunAICLICommand("uninstall", $"--product-code {productCode}");

            Assert.AreNotEqual(Constants.ErrorCode.S_OK, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Portable package from a different source already exists"));
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            // Modify packageId and sourceId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetSourceIdentifier, "testSourceId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0 --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Successfully installed"));
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 3.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.APPINSTALLER_CLI_ERROR_INSTALL_UPGRADE_NOT_SUPPORTED, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("package cannot be upgraded using winget"));
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.User);
        }

        /// <summary>
        /// Test upgrade when a new dependency is added that is not installed.
        /// </summary>
        [Test]
        public void UpgradeAddsDependency()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestUpgradeAddsDependency -v 1.0 --verbose");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("upgrade", $"AppInstallerTest.TestUpgradeAddsDependency");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
        }
    }
}
