// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
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

        [Test]
        public void InstallExeWithAlternateSourceFailure()
        {
            TestCommon.RunAICLICommand("source add", "failSearch \"{ \"\"SearchHR\"\": \"\"0x80070002\"\" }\" Microsoft.Test.Configurable --header \"{}\"");

            try
            {
                var installDir = TestCommon.GetRandomTestDir();
                var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
                Assert.AreEqual(unchecked((int)0x80070002), result.ExitCode);
                Assert.True(result.StdOut.Contains("Failed when searching source: failSearch"));
                Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
                Assert.False(result.StdOut.Contains("Successfully installed"));
                Assert.False(VerifyTestExeInstalled(installDir));
            }
            finally
            {
                ResetTestSource();
            }
        }

        [Test]
        public void InstallPortableExe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, commandAlias, fileName, productCode;
            packageId = commandAlias = productCode = "AppInstallerTest.TestPortableExe";
            fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestPortableExe -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestPortableInstalledAndCleanup(installDir, packageId, commandAlias, fileName, productCode));
        }

        [Test]
        public void InstallPortableExeWithCommand()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, commandAlias, fileName, productCode;
            packageId = productCode = "AppInstallerTest.TestPortableExeWithCommand";
            fileName = "AppInstallerTest.TestPortableExe";
            commandAlias = "testCommand";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestPortableInstalledAndCleanup(installDir, packageId, commandAlias, fileName, productCode));
        }

        [Test]
        public void InstallPortableExeWithAppsAndFeatures()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            commandAlias = fileName = "AppInstallerTest.TestPortableExe";
            productCode = "testProductCode";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestPortableInstalledAndCleanup(installDir, packageId, commandAlias, fileName, productCode));
        }

        [Test]
        public void InstallPortableExeWithRename()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string renameArgValue = "testRename";
            string packageId, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithRename";
            productCode = "testProductCode";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir} -r {renameArgValue}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(VerifyTestPortableInstalledAndCleanup(installDir, packageId, renameArgValue, renameArgValue, productCode));
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

        private bool VerifyTestPortableInstalledAndCleanup(
            string installDir,
            string packageId,
            string expectedCommandAlias,
            string expectedFileName,
            string expectedProductCode)
        {
            string installPackageRoot = Path.Combine(installDir, packageId);
            if (!File.Exists(Path.Combine(installPackageRoot, expectedFileName)))
            {
                return false;
            }
            else
            {
                DirectoryInfo di = new DirectoryInfo(installDir);
                foreach (FileInfo file in di.GetFiles())
                {
                    file.Delete();
                }

                foreach (DirectoryInfo dir in di.GetDirectories())
                {
                    dir.Delete(true);
                }
            }

            string symlinkPath = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Links", expectedCommandAlias);
            if (!File.Exists(symlinkPath))
            {
                return false;
            }
            else
            {
                File.Delete(symlinkPath);
            }

            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall"))
            {
                var installTestPortableSubkey = uninstallRegistryKey.OpenSubKey(expectedProductCode);
                if (installTestPortableSubkey == null)
                {
                    return false;
                }
                else
                {
                    uninstallRegistryKey.DeleteSubKey(expectedProductCode);
                }
            }

            using (RegistryKey environmentRegistryKey = Registry.CurrentUser.OpenSubKey(@"Environment"))
            {
                var currentPathValue = (string)environmentRegistryKey.GetValue("Path");
                var portablePathValue = installPackageRoot + ';';
                if (!currentPathValue.Contains(portablePathValue))
                {
                    return false;
                }
                else
                {
                    string initialPathValue = currentPathValue.Replace(portablePathValue, "");
                    environmentRegistryKey.SetValue("Path", initialPathValue);
                }
            }

            return true;
        }
    }
}