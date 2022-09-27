// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class InstallCommand : BaseCommand
    {
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            ConfigureFeature("zipInstall", true);
        }

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
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        [Test]
        public void InstallExeWithInsufficientMinOsVersion()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");
            // MinOSVersion is moved to installer level, the check is performed during installer selection
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER, result.ExitCode);
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallExeWithHashMismatch()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallWithInno()
        {
            // Install test inno, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestInnoInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/VERYSILENT"));
        }

        [Test]
        public void InstallBurn()
        {
            // Install test burn, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestBurnInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/quiet"));
        }

        [Test]
        public void InstallNullSoft()
        {
            // Install test Nullsoft, manifest does not provide silent switch, we should be populating the default
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestNullsoftInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/S"));
        }

        [Test]
        public void InstallMSI()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallMSIX()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallMSIXWithSignature()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public void InstallMSIXWithSignatureHashMismatch()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(TestCommon.VerifyTestMsixInstalledAndCleanup());
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
                Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
            }
            finally
            {
                ResetTestSource();
            }
        }

        [Test]
        public void InstallPortableExe()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            // If no location specified, default behavior is to create a package directory with the name "{packageId}_{sourceId}"
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortableExeWithCommand()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            fileName = "AppInstallerTestExeInstaller.exe";
            commandAlias = "testCommand.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(installDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortableExeWithRename()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, productCode, renameArgValue;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            renameArgValue = "testRename.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir} --rename {renameArgValue}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(installDir, renameArgValue, renameArgValue, productCode, true);
        }

        [Test]
        public void InstallPortableInvalidRename()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, renameArgValue;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            renameArgValue = "test?";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir} --rename {renameArgValue}");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("The specified filename is not a valid filename"));
        }

        [Test]
        public void InstallPortableReservedNames()
        {
            var installDir = TestCommon.GetRandomTestDir();
            string packageId, renameArgValue;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            renameArgValue = "CON";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {installDir} --rename {renameArgValue}");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("The specified filename is not a valid filename"));
        }

        [Test]
        public void InstallPortableToExistingDirectory()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var existingDir = Path.Combine(installDir, "testDirectory");
            Directory.CreateDirectory(existingDir);

            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -l {existingDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(existingDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortableFailsWithCleanup()
        {
            string packageId, commandAlias;
            packageId = "AppInstallerTest.TestPortableExe";
            commandAlias = "AppInstallerTestExeInstaller.exe";

            // Create a directory with the same name as the symlink in order to cause install to fail.
            string symlinkDirectory = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string conflictDirectory = Path.Combine(symlinkDirectory, commandAlias);

            Directory.CreateDirectory(conflictDirectory);

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");

            // Remove directory prior to assertions as this will impact other tests if assertions fail.
            Directory.Delete(conflictDirectory, true);

            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Unable to create symlink, path points to a directory."));
        }

        [Test]
        public void ReinstallPortable()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            string symlinkDirectory = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Clean first install should not display file overwrite message.
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.False(result.StdOut.Contains($"Overwriting existing file: {symlinkPath}"));

            // Perform second install and verify that file overwrite message is displayed.
            var result2 = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortable_UserScope()
        {
            string installDir = TestCommon.GetRandomTestDir();
            ConfigureInstallBehavior(Constants.PortablePackageUserRoot, installDir);

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} --scope user");
            ConfigureInstallBehavior(Constants.PortablePackageUserRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortable_MachineScope()
        {
            string installDir = TestCommon.GetRandomTestDir();
            ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, installDir);

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} --scope machine");
            ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.Machine);
        }

        [Test]
        public void InstallZip_Exe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithExe --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        [Test]
        public void InstallZip_Portable()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestZipInstallerWithPortable";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = "TestPortable.exe";
            fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.User);
        }

        [Test]
        public void InstallZipWithInvalidRelativeFilePath()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInvalidRelativePath");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Invalid relative file path to the nested installer; path points to a location outside of the install directory"));
        }

        [Test]
        public void InstallZipWithMsi()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsi --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallZipWithMsix()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsix");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }
    }
}