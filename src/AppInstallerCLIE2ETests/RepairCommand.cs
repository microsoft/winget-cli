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
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            // Note: The 'msiexec repair' command requires the original installer file to be present at the location registered in the ARP (Add/Remove Programs).
            // In our test scenario, the MSI installer file is initially placed in a temporary location and then deleted, which can cause the repair operation to fail.
            // To work around this, we copy the installer file to the ARP source directory before running the repair command.
            // A more permanent solution would be to modify the MSI installer to cache the installer file in a known location and register that location as the installer source.
            // This would allow the 'msiexec repair' command to function as expected.
            string installerSourceDir = TestCommon.CopyInstallerFileToARPInstallSourceDirectory(TestCommon.GetTestDataFile("AppInstallerTestMsiInstallerV2.msi"), Constants.MsiInstallerProductCode, true);

            result = TestCommon.RunAICLICommand("repair", $"AppInstallerTest.TestMsiRepair");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));

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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestModifyRepair");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Modify Repair operation"));
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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.UninstallerRepair");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Uninstaller Repair operation"));
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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.UninstallerRepair");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Uninstaller Repair operation"));
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

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            result = TestCommon.RunAICLICommand("repair", "AppInstallerTest.TestInstallerRepair");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Repair operation completed successfully"));
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(installDir, "Installer Repair operation"));
        }
    }
}
