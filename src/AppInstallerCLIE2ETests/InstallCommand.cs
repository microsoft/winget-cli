// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class InstallCommand
    {
        // Todo: add unicode test cases after install tests are enabled.
        private const string InstallTestSourceUrl = @"https://localhost:5001/TestKit/";
        private const string InstallTestSourceName = @"InstallTestSource";

        private const string InstallTestExeInstalledFile = @"TestExeInstalled.txt";
        private const string InstallTestMsiInstalledFile = @"AppInstallerTestExeInstaller.exe";
        private const string InstallTestMsiProductId = @"{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        private const string InstallTestMsixName = @"6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        [SetUp]
        public void Setup()
        {
           Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{InstallTestSourceName} {InstallTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", InstallTestSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void AppToInstallDoesNotExist()
        {
            // Cannot find an app to install
            var result = TestCommon.RunAICLICommand("install", "DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void MultipleAppsMatchQuery()
        {
            // Too many apps match the query
            var result = TestCommon.RunAICLICommand("install", "TestMultipleAppFoundInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple packages found matching input criteria. Please refine the input."));
        }

        [Test]
        public void InstallTestExe()
        {
            // Install test exe
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/execustom"));
        }

        [Test]
        public void InstallTestExeWithInsufficientMinOsVersion()
        {
            // Install test exe but min os version too high
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_OLD_WIN_VERSION, result.ExitCode);
            Assert.True(result.StdOut.Contains("Cannot install package, as it requires a higher version of Windows"));
            Assert.False(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public void ExeInstallWithHashMismatchPassWithNToFail()
        {
            // Install test exe but hash mismatch, passing N should cause the installation to fail
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "N");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public void ExeInstallWithHashMismatchPassWithYToContinue()
        {
            // Install test exe but hash mismatch, passing Y should cause the installation to continue
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "Y");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/execustom"));
        }

        [Test]
        public void InstallWithInno()
        {
            // Install test inno, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestInnoInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/VERYSILENT"));
        }

        [Test]
        public void InstallTestBurn()
        {
            // Install test burn, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestBurnInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/quiet"));
        }

        [Test]
        public void InstallTestNullSoft()
        {
            // Install test Nullsoft, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestNullsoftInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/S"));
        }

        [Test]
        public void InstallTestMSI()
        {
            // Install test msi
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallTestMSIX()
        {
            // Install test msix
            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallTestMSIXWithSignature()
        {
            // Install test msix with signature provided
            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallTestMSIXWithSignatureHashMismatchPassNToFail()
        {
            // Install test msix with signature hash mismatch, passing N should cause the installation to fail
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "N");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash mismatch"));
            Assert.False(VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallTestMSIXWithSignatureHashMismatchPassYToContinue()
        {
            // Install test msix with signature hash mismatch, passing Y should cause the installation to continue
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "Y");
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