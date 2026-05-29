// -----------------------------------------------------------------------------
// <copyright file="RepairCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test Repair command.
    /// </summary>
    public class RepairCommand : BaseCommand
    {
        /// <summary>
        /// One time setup.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            // Try clean up AppInstallerTest.TestMsiInstaller for failure cases where cleanup is not successful
            TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestMsiInstaller");
        }

        /// <summary>
        /// Test  MSI installer repair.
        /// </summary>
        [Test]
        public void RepairMSIInstaller()
        {
            if (string.IsNullOrEmpty(TestIndex.MsiInstallerV2))
            {
                Assert.Ignore("MSI installer not available");
            }

            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMsiRepair --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            // Note: The 'msiexec repair' command requires the original installer file to be present at the location registered in the ARP (Add/Remove Programs).
            // In our test scenario, the MSI installer file is initially placed in a temporary location and then deleted, which can cause the repair operation to fail.
            // To work around this, we copy the installer file to the ARP source directory before running the repair command.
            // A more permanent solution would be to modify the MSI installer to cache the installer file in a known location and register that location as the installer source.
            // This would allow the 'msiexec repair' command to function as expected.
            string installerSourceDir = TestCommon.CopyInstallerFileToARPInstallSourceDirectory(TestCommon.GetTestDataFile("AppInstallerTestMsiInstallerV2.msi"), Constants.MsiInstallerProductCode, true);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestMsiRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir), Is.True);

            if (installerSourceDir != null && Directory.Exists(installerSourceDir))
            {
                Directory.Delete(installerSourceDir, true);
            }
        }

        /// <summary>
        /// Test  MSIX non-store package repair.
        /// </summary>
        [Test]
        public void RepairNonStoreMSIXPackage()
        {
            // install a test msix package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMsixInstaller --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestMsixInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(), Is.True);
        }

        /// <summary>
        /// Test MSIX non-store package repair with machine scope.
        /// </summary>
        [Test]
        public void RepairNonStoreMsixPackageWithMachineScope()
        {
            // Selecting Microsoft.Paint_8wekyb3d8bbwe because it's a system package suitable for this scenario.
            // First, we need to ensure this package is installed, otherwise, we skip the test.
            var result = TestCommon.RunAICLICommand("list", "Microsoft.Paint_8wekyb3d8bbwe");

            if (result.ExitCode != Constants.ErrorCode.S_OK)
            {
                Assert.Ignore("Test skipped as Microsoft.Paint_8wekyb3d8bbwe is not installed.");
            }

            Assert.That(result.StdOut.Contains("Microsoft.Paint"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "Microsoft.Paint_8wekyb3d8bbwe --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
            Assert.That(result.StdOut.Contains("The current system configuration does not support the repair of this package."), Is.True);
        }

        /// <summary>
        /// Test repair of a Burn installer that has a "modify" repair behavior specified in the manifest.
        /// </summary>
        [Test]
        public void RepairBurnInstallerWithModifyBehavior()
        {
            // install a test burn package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestModifyRepair -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestModifyRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Modify Repair operation"), Is.True);
        }

        /// <summary>
        /// Tests the repair operation of a Burn installer that was installed in user scope but is being repaired in an admin context.
        /// </summary>
        [Test]
        public void RepairBurnInstallerInAdminContextWithUserScopeInstall()
        {
            // install a test burn package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestUserScopeInstallRepairInAdminContext -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestUserScopeInstallRepairInAdminContext");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_ADMIN_CONTEXT_REPAIR_PROHIBITED));
            Assert.That(result.StdOut.Contains("The package installed for user scope cannot be repaired when running with administrator privileges."), Is.True);
            TestCommon.CleanupTestExeAndDirectory(installDir);
        }

        /// <summary>
        /// Tests the repair operation of a Burn installer that lacks a repair behavior.
        /// </summary>
        [Test]
        public void RepairBurnInstallerMissingRepairBehavior()
        {
            // install a test burn package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMissingRepairBehavior -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestMissingRepairBehavior");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_REPAIR_INFO_FOUND));
            Assert.That(result.StdOut.Contains("The repair command for this package cannot be found. Please reach out to the package publisher for support."), Is.True);
            TestCommon.CleanupTestExeAndDirectory(installDir);
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest and NoModify ARP flag set.
        /// </summary>
        [Test]
        public void RepairBurnInstallerWithWithModifyBehaviorAndNoModifyFlag()
        {
            // install a test Exe package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestModifyRepairWithNoModify -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestModifyRepairWithNoModify");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED));
            Assert.That(result.StdOut.Contains("The installer technology in use does not support repair."), Is.True);
            TestCommon.CleanupTestExeAndDirectory(installDir);
        }

        /// <summary>
        /// Tests the scenario where the repair operation is not supported for Portable Installer type.
        /// </summary>
        [Test]
        public void RepairOperationNotSupportedForPortableInstaller()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestPortableExe");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED));
            Assert.That(result.StdOut.Contains("The installer technology in use does not support repair."), Is.True);

            // If no location specified, default behavior is to create a package directory with the name "{packageId}_{sourceId}"
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest.
        /// </summary>
        [Test]
        public void RepairExeInstallerWithUninstallerBehavior()
        {
            // install a test Exe package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.UninstallerRepair -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.UninstallerRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Uninstaller Repair operation"), Is.True);
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest and NoRepair ARP flag set.
        /// </summary>
        [Test]
        public void RepairExeInstallerWithUninstallerBehaviorAndNoRepairFlag()
        {
            // install a test Exe package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.UninstallerRepairWithNoRepair -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.UninstallerRepairWithNoRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED));
            Assert.That(result.StdOut.Contains("The installer technology in use does not support repair."), Is.True);
            TestCommon.CleanupTestExeAndDirectory(installDir);
        }

        /// <summary>
        /// Test repair of a Nullsoft installer that has a "uninstaller" repair behavior specified in the manifest.
        /// </summary>
        [Test]
        public void RepairNullsoftInstallerWithUninstallerBehavior()
        {
            // install a test Nullsoft package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.UninstallerRepair -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.UninstallerRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Uninstaller Repair operation"), Is.True);
        }

        /// <summary>
        /// Test repair of a Inno installer that has a "installer" repair behavior specified in the manifest.
        /// </summary>
        [Test]
        public void RepairInnoInstallerWithInstallerBehavior()
        {
            // install a test Inno package from TestSource and then repair it.
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestInstallerRepair -v 2.0.0.0 --silent -l {installDir}");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestInstallerRepair");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Repair operation completed successfully"), Is.True);
            Assert.That(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Installer Repair operation"), Is.True);
        }
    }
}
