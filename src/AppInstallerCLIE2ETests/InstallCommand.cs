// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class InstallCommand
    {
        // Todo: this should point to a loopback address. Disabling the install tests until we have loopback support done in our e2e tests.
        // Todo: add unicode test cases after install tests are enabled.
        private const string InstallTestSourceUrl = @"https://github.com/microsoft/appinstaller-cli/raw/master/src/AppInstallerCLIE2ETests/TestData";
        private const string InstallTestSourceName = @"InstallTestSource";

        private const string InstallTestExeInstalledFile = @"TestExeInstalled.txt";
        private const string InstallTestMsiInstalledFile = @"AppInstallerTestExeInstaller.exe";
        private const string InstallTestMsiProductId = @"{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        private const string InstallTestMsixName = @"6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        //[SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{InstallTestSourceName} {InstallTestSourceUrl}").ExitCode);
        }

        //[TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", InstallTestSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        //[Test]
        public void InstallCommands()
        {
            // Cannot find an app to install
            var result = TestCommon.RunAICLICommand("install", "DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No app found matching input criteria."));

            // Too many apps match the query
            result = TestCommon.RunAICLICommand("install", "AppInstallerTest");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple apps found matching input criteria. Please refine the input."));

            // Install test exe
            var installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/execustom"));

            // Install test exe but min os version too high
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_OLD_WIN_VERSION, result.ExitCode);
            Assert.True(result.StdOut.Contains("Cannot install application, as it requires a higher version of Windows"));
            Assert.False(VerifyTestExeInstalled(installDir));

            // Install test exe but hash mismatch, passing N should cause the installation to fail
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "N");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash mismatch"));
            Assert.False(VerifyTestExeInstalled(installDir));

            // Install test exe but hash mismatch, passing Y should cause the installation to continue
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "Y");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/execustom"));

            // Install test inno, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestInnoInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/VERYSILENT"));

            // Install test burn, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestBurnInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/quiet"));

            // Install test Nullsoft, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestNullsoftInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/S"));

            // Install test msi
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsiInstalledAndCleanup(installDir));

            // Install test msix
            result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature provided
            result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature hash mismatch, passing N should cause the installation to fail
            result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "N");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash mismatch"));
            Assert.False(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature hash mismatch, passing Y should cause the installation to continue
            result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "Y");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());
        }

        private bool VerifyTestExeInstalled(string installDir, string expectedContent = null)
        {
            if (!File.Exists(Path.Combine(installDir, InstallTestExeInstalledFile)))
            {
                return false;
            }

            if (!string.IsNullOrEmpty(expectedContent))
            {
                string content = File.ReadAllText(Path.Combine(installDir, InstallTestExeInstalledFile));
                return content.Contains(expectedContent);
            }

            return true;
        }

        private bool VerifyTestMsiInstalledAndCleanup(string installDir)
        {
            if (!File.Exists(Path.Combine(installDir, InstallTestMsiInstalledFile)))
            {
                return false;
            }

            return TestCommon.RunCommand("msiexec.exe", $"/qn /x {InstallTestMsiProductId}");
        }

        private bool VerifyTestMsixInstalledAndCleanup()
        {
            var result = TestCommon.RunCommandWithResult("powershell", $"Get-AppxPackage {InstallTestMsixName}");

            if (!result.StdOut.Contains(InstallTestMsixName))
            {
                return false;
            }

            return TestCommon.RemoveMsix(InstallTestMsixName);
        }
    }
}