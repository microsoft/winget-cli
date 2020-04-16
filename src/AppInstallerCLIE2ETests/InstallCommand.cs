// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class InstallCommand
    {
        private const string InstallTestSourceUrl = @"https://github.com/microsoft/appinstaller-cli/raw/master/src/AppInstallerCLIE2ETests/TestData";
        private const string InstallTestSourceName = @"InstallTestSource";

        private const string InstallTestExeInstalledFile = @"TestExeInstalled.txt";
        private const string InstallTestMsiInstalledFile = @"AppInstallerTestExeInstaller.exe";
        private const string InstallTestMsiProductId = @"{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        private const string InstallTestMsixName = @"6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        /*[SetUp]
        public void Setup()
        {
            Assert.AreEqual(TestCommon.RunAICLICommand("source", $"add {InstallTestSourceName} {InstallTestSourceUrl}").ExitCode, 0);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source", $"remove {InstallTestSourceName}");
            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void InstallCommands()
        {
            // Cannot find an app to install
            var result = TestCommon.RunAICLICommand("install", "DoesNotExist");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));

            // Too many apps match the query
            result = TestCommon.RunAICLICommand("install", "AppInstallerTest");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));

            // Install test exe
            var installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeInstaller --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestExeInstalled(installDir));

            // Install test exe but min os version too high
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.False(VerifyTestExeInstalled(installDir));

            // Install test exe but hash mismatch, passing N should cause the installation to fail
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "N");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.False(VerifyTestExeInstalled(installDir));

            // Install test exe but hash mismatch, passing Y should cause the installation to continue
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}", "Y");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestExeInstalled(installDir));

            // Install test inno, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestInnoInstaller --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestExeInstalled(installDir, "/VERYSILENT"));

            // Install test burn, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestBurnInstaller --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestExeInstalled(installDir, "/quiet"));

            // Install test Nullsoft, manifest does not provide silent switch, we should be populating the default
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestNullsoftInstaller --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestExeInstalled(installDir, "/S"));

            // Install test msi
            installDir = TestCommon.GetRandomTestDir();
            result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestMsiInstalledAndCleanup(installDir));

            // Install test msix
            result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature provided
            result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.True(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature hash mismatch, passing N should cause the installation to fail
            result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "N");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
            Assert.False(VerifyTestMsixInstalledAndCleanup());

            // Install test msix with signature hash mismatch, passing Y should cause the installation to continue
            result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch", "N");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
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
        }*/
    }
}