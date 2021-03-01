// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class InstallCommand : BaseCommand
    {
        private const string InstallTestMsiInstalledFile = @"AppInstallerTestExeInstaller.exe";
        private const string InstallTestMsiProductId = @"{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        private const string InstallTestMsixName = @"6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        [Test]
        public void InstallAppDoesNotExist()
        {
            var result = TestCommon.RunAICLICommand("install", "DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            var result = TestCommon.RunAICLICommand("install", "TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple packages found matching input criteria. Please refine the input."));
        }

        [Test]
        public void InstallExe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/execustom"));
        }

        [Test]
        public void InstallExeWithInsufficientMinOsVersion()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");
            // MinOSVersion is moved to installer level, the check is performed during installer selection
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER, result.ExitCode);
            Assert.False(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public void InstallExeWithHashMismatch()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(VerifyTestExeInstalled(installDir));
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
        public void InstallBurn()
        {
            // Install test burn, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestBurnInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/quiet"));
        }

        [Test]
        public void InstallNullSoft()
        {
            // Install test Nullsoft, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestNullsoftInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestExeInstalled(installDir, "/S"));
        }

        [Test]
        public void InstallMSI()
        {
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallMSIX()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallMSIXWithSignature()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallMSIXWithSignatureHashMismatch()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(VerifyTestMsixInstalledAndCleanup());
        }

        private bool VerifyTestExeInstalled(string installDir, string expectedContent = null)
        {
            if (!File.Exists(Path.Combine(installDir, Constants.TestExeInstalledFileName)))
            {
                return false;
            }

            if (!string.IsNullOrEmpty(expectedContent))
            {
                string content = File.ReadAllText(Path.Combine(installDir, Constants.TestExeInstalledFileName));
                return content.Contains(expectedContent);
            }

            TestCommon.RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
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