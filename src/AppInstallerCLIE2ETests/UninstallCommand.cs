// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class UninstallCommand : BaseCommand
    {
        // Custom product code for overriding the default in the test exe
        private const string CustomProductCode = "{f08fc03c-0b7e-4fca-9b3c-3a384d18a9f3}";

        // File written when uninstalling the test exe
        private const string UninstallTestExeUninstalledFile = "TestExeUninstalled.txt";

        // Name of a file installed by the MSI that will be removed during uninstall
        private const string UninstallTestMsiInstalledFile = "AppInstallerTestExeInstaller.exe";

        // Package name of the test MSIX package
        private const string UninstallTestMsixName = "6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            InitializeAllFeatures(false);
            ConfigureFeature("uninstall", true);
        }

        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            InitializeAllFeatures(false);
        }

        [Test]
        public void UninstallTestExe()
        {
            // Uninstall an Exe
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.ExeInstallerPackageId} --silent -l {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", Constants.ExeInstallerPackageId);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully uninstalled"));
            Assert.True(VerifyTestExeUninstalled(installDir));
        }

        [Test]
        public void UninstallTestMsi()
        {
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Uninstall an MSI
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.MsiInstallerPackageId} -l {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", Constants.MsiInstallerPackageId);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully uninstalled"));
            Assert.True(VerifyTestMsiUninstalled(installDir));
        }

        [Test]
        public void UninstallTestMsix()
        {
            // Uninstall an MSIX
            TestCommon.RunAICLICommand("install", Constants.MsixInstallerPackageId);
            var result = TestCommon.RunAICLICommand("uninstall", Constants.MsixInstallerPackageId);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully uninstalled"));
            Assert.True(VerifyTestMsixUninstalled());
        }

        [Test]
        public void UninstallNotIndexed()
        {
            // Uninstalls a package found with ARP not matching any known manifest.
            // Install the test EXE providing a custom Product Code so that it cannot be mapped
            // back to its manifest, then uninstall it using its Product Code
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.ExeInstallerPackageId} --override \"/ProductID {CustomProductCode} /InstallDir {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", CustomProductCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully uninstalled"));
            Assert.True(VerifyTestExeUninstalled(installDir));
        }

        [Test]
        public void UninstallAppNotInstalled()
        {
            // Verify failure when trying to uninstall an app that is not installed.
            var result = TestCommon.RunAICLICommand("uninstall", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No installed package found matching input criteria."));
        }

        private bool VerifyTestExeUninstalled(string installDir)
        {
            return File.Exists(Path.Combine(installDir, UninstallTestExeUninstalledFile));
        }

        private bool VerifyTestMsiUninstalled(string installDir)
        {
            return !File.Exists(Path.Combine(installDir, UninstallTestMsiInstalledFile));
        }

        private bool VerifyTestMsixUninstalled()
        {
            var result = TestCommon.RunCommandWithResult("powershell", $"Get-AppxPackage {UninstallTestMsixName}");
            return string.IsNullOrWhiteSpace(result.StdOut);
        }
    }
}
