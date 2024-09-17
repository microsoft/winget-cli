// -----------------------------------------------------------------------------
// <copyright file="InstallCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test install command.
    /// </summary>
    public class InstallCommand : BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // Try clean up TestExeInstaller for failure cases where cleanup is not successful
            TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestExeInstaller");
        }

        /// <summary>
        /// Test package doesn't exist.
        /// </summary>
        [Test]
        public void InstallAppDoesNotExist()
        {
            var result = TestCommon.RunAICLICommand("install", "DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test multiple matches found.
        /// </summary>
        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            var result = TestCommon.RunAICLICommand("install", "TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple packages found matching input criteria. Please refine the input."));
        }

        /// <summary>
        /// Test install exe.
        /// </summary>
        [Test]
        public void InstallExe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        /// <summary>
        /// Test inapplicable os version.
        /// </summary>
        [Test]
        public void InstallExeWithInsufficientMinOsVersion()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"InapplicableOsVersion --silent -l {installDir}");

            // MinOSVersion is moved to installer level, the check is performed during installer selection
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER, result.ExitCode);
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
        }

        /// <summary>
        /// Test install exe hash mismatch.
        /// </summary>
        [Test]
        public void InstallExeWithHashMismatch()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
        }

        /// <summary>
        /// Test install inno.
        /// </summary>
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

        /// <summary>
        /// Test install burn.
        /// </summary>
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

        /// <summary>
        /// Test install nullsoft.
        /// </summary>
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

        /// <summary>
        /// Test install msi.
        /// </summary>
        [Test]
        public void InstallMSI()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        /// <summary>
        /// Test install msix.
        /// </summary>
        [Test]
        public void InstallMSIX()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test install msix machine scope.
        /// </summary>
        [Test]
        public void InstallMSIXMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller --scope machine");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup(true));
        }

        /// <summary>
        /// Test install msix with signature hash.
        /// </summary>
        [Test]
        public void InstallMSIXWithSignature()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test install msix with signature hash machine scope.
        /// </summary>
        [Test]
        public void InstallMSIXWithSignatureMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash --scope machine");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup(true));
        }

        /// <summary>
        /// Test msix hash mismatch.
        /// </summary>
        [Test]
        public void InstallMSIXWithSignatureHashMismatch()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, result.ExitCode);
            Assert.True(result.StdOut.Contains("Installer hash does not match"));
            Assert.False(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test install with alternate source failure.
        /// </summary>
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
                this.ResetTestSource();
            }
        }

        /// <summary>
        /// Test install portable package.
        /// </summary>
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

        /// <summary>
        /// Test install portable package with command.
        /// </summary>
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

        /// <summary>
        /// Test install portable package with rename.
        /// </summary>
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

        /// <summary>
        /// Test install portable package invalid rename.
        /// </summary>
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

        /// <summary>
        /// Test install portable package with reserve names.
        /// </summary>
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

        /// <summary>
        /// Test install portable package to an existing directory.
        /// </summary>
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

        /// <summary>
        /// Test install portable package. Symlink is a directory.
        /// </summary>
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

        /// <summary>
        /// Test reinstalling portable package.
        /// </summary>
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
            var result2 = TestCommon.RunAICLICommand("install", $"{packageId} --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test installing portable package user scope.
        /// </summary>
        [Test]
        public void InstallPortable_UserScope()
        {
            string installDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageUserRoot, installDir);

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} --scope user");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageUserRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test install portable package machine scope.
        /// </summary>
        [Test]
        public void InstallPortable_MachineScope()
        {
            string installDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, installDir);

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} --scope machine");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.Machine);
        }

        /// <summary>
        /// Test install portable package with settings set to user install scope.
        /// </summary>
        [Test]
        public void InstallPortable_InstallScopePreference_User()
        {
            string installDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageUserRoot, installDir);
            WinGetSettingsHelper.ConfigureInstallBehaviorPreferences(Constants.InstallBehaviorScope, "user");

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageUserRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test install portable package with settings set to machine install scope.
        /// </summary>
        [Test]
        public void InstallPortable_InstallScopePreference_Machine()
        {
            string installDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, installDir);
            WinGetSettingsHelper.ConfigureInstallBehaviorPreferences(Constants.InstallBehaviorScope, "machine");

            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.PortablePackageMachineRoot, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.Machine);
        }

        /// <summary>
        /// Test install zip exe.
        /// </summary>
        [Test]
        public void InstallZip_Exe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithExe --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        /// <summary>
        /// Test install zip portable.
        /// </summary>
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

        /// <summary>
        /// Test install zip portable with binaries that depend on PATH variable.
        /// </summary>
        [Test]
        public void InstallZip_ArchivePortableWithBinariesDependentOnPath()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.ArchivePortableWithBinariesDependentOnPath";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = "TestPortable.exe";
            fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.User, true);
        }

        /// <summary>
        /// Test install zip with invalid relative file path.
        /// </summary>
        [Test]
        public void InstallZipWithInvalidRelativeFilePath()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInvalidRelativePath");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Invalid relative file path to the nested installer; path points to a location outside of the install directory"));
        }

        /// <summary>
        /// Test install zip msi.
        /// </summary>
        [Test]
        public void InstallZipWithMsi()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsi --silent -l {installDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        /// <summary>
        /// Test install zip msix.
        /// </summary>
        [Test]
        public void InstallZipWithMsix()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsix");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test install zip exe by extracting with tar.
        /// </summary>
        [Test]
        public void InstallZip_ExtractWithTar()
        {
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.ArchiveExtractionMethod, "tar");
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithExe --silent -l {installDir}");
            WinGetSettingsHelper.ConfigureInstallBehavior(Constants.ArchiveExtractionMethod, string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        /// <summary>
        /// Test install an installed package and convert to upgrade.
        /// </summary>
        [Test]
        public void InstallExeFoundExistingConvertToUpgrade()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 1.0.0.0 --silent -l {baseDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, baseResult.ExitCode);
            Assert.True(baseResult.StdOut.Contains("Successfully installed"));

            // Install will convert to upgrade
            var upgradeDir = TestCommon.GetRandomTestDir();
            var upgradeResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {upgradeDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Trying to upgrade the installed package..."));
            Assert.True(upgradeResult.StdOut.Contains("Successfully installed"));

            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(upgradeDir, "/Version 2.0.0.0"));
        }

        /// <summary>
        /// Test install an installed package without an available upgrade.
        /// </summary>
        [Test]
        public void InstallExeFoundExistingConvertToUpgradeNoAvailableUpgrade()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 2.0.0.0 --silent -l {baseDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, baseResult.ExitCode);
            Assert.True(baseResult.StdOut.Contains("Successfully installed"));

            // Install will convert to upgrade
            var upgradeDir = TestCommon.GetRandomTestDir();
            var upgradeResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {upgradeDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_UPDATE_NOT_APPLICABLE, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Trying to upgrade the installed package..."));
            Assert.True(upgradeResult.StdOut.Contains("No available upgrade"));

            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir));
        }

        /// <summary>
        /// Test force installing a package.
        /// </summary>
        [Test]
        public void InstallExeWithLatestInstalledWithForce()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 2.0.0.0 --silent -l {baseDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, baseResult.ExitCode);
            Assert.True(baseResult.StdOut.Contains("Successfully installed"));

            // Install will not convert to upgrade
            var installDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 1.0.0.0 --silent -l {installDir} --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        /// <summary>
        /// Test install a package with an invalid Windows Feature dependency.
        /// </summary>
        [Test]
        public void InstallWithWindowsFeatureDependency_FeatureNotFound()
        {
            var testDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.WindowsFeature -l {testDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALL_DEPENDENCIES, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("The feature [invalidFeature] was not found."));
            Assert.True(installResult.StdOut.Contains("Failed to enable Windows Feature dependencies. To proceed with installation, use '--force'."));
        }

        /// <summary>
        /// Test install a package with a Windows Feature dependency using the force argument.
        /// </summary>
        [Test]
        public void InstallWithWindowsFeatureDependency_Force()
        {
            var testDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.WindowsFeature --silent --force -l {testDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Failed to enable Windows Feature dependencies; proceeding due to --force"));
            Assert.True(installResult.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(testDir));
        }

        /// <summary>
        /// Test install a package with a package dependency that requires the PATH environment variable to be refreshed between dependency installs.
        /// </summary>
        [Test]
        public void InstallWithPackageDependency_RefreshPathVariable()
        {
            var testDir = TestCommon.GetRandomTestDir();
            string installDir = TestCommon.GetPortablePackagesDirectory();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.PackageDependencyRequiresPathRefresh -l {testDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            // Portable package is used as a dependency. Ensure that it is installed and cleaned up successfully.
            string portablePackageId, commandAlias, fileName, packageDirName, productCode;
            portablePackageId = "AppInstallerTest.TestPortableExeWithCommand";
            packageDirName = productCode = portablePackageId + "_" + Constants.TestSourceIdentifier;
            fileName = "AppInstallerTestExeInstaller.exe";
            commandAlias = "testCommand.exe";

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(testDir));
        }

        /// <summary>
        /// Test install a package using a specific installer type.
        /// </summary>
        [Test]
        public void InstallWithInstallerTypeArgument()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMultipleInstallers --silent -l {installDir} --installer-type exe");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));
        }

        /// <summary>
        /// Test install package with installer type preference settings.
        /// </summary>
        [Test]
        public void InstallWithInstallerTypePreference()
        {
            string[] installerTypePreference = { "nullsoft" };
            WinGetSettingsHelper.ConfigureInstallBehaviorPreferences(Constants.InstallerTypes, installerTypePreference);

            string installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMultipleInstallers --silent -l {installDir}");

            // Reset installer type preferences.
            WinGetSettingsHelper.ConfigureInstallBehaviorPreferences(Constants.InstallerTypes, Array.Empty<string>());

            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), "/S");
        }

        /// <summary>
        /// Test install package with installer type requirement settings.
        /// </summary>
        [Test]
        public void InstallWithInstallerTypeRequirement()
        {
            string[] installerTypeRequirement = { "inno" };
            WinGetSettingsHelper.ConfigureInstallBehaviorRequirements(Constants.InstallerTypes, installerTypeRequirement);

            string installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMultipleInstallers --silent -l {installDir}");

            // Reset installer type requirements.
            WinGetSettingsHelper.ConfigureInstallBehaviorRequirements(Constants.InstallerTypes, Array.Empty<string>());

            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER, result.ExitCode);
            Assert.True(result.StdOut.Contains("No applicable installer found; see logs for more details."));
        }

        /// <summary>
        /// This test flow is intended to test an EXE that actually installs an MSIX internally, and whose name+publisher
        /// information resembles an existing installation. Given this, the goal is to get correlation to stick to the
        /// MSIX rather than the ARP entry that we would match with in the absence of the package family name being present.
        /// </summary>
        [Test]
        public void InstallExeThatInstallsMSIX()
        {
            string targetPackageIdentifier = "AppInstallerTest.TestExeInstallerInstallsMSIX";
            string fakeProductCode = "e35f5799-cce3-41fd-886c-c36fcb7104fe";

            // Insert fake ARP entry as if a non-MSIX version of the package is already installed.
            // The name here must not match the normalized name of the package, but be close enough to meet
            // the confidence requirements for correlation after an install operation (so we drop one word here).
            TestCommon.CreateARPEntry(fakeProductCode, new
            {
                DisplayName = "EXE Installer that Installs MSIX",
                Publisher = "AppInstallerTest",
                DisplayVersion = "1.0.0",
            });

            // We should not find it before installing because the normalized name doesn't match
            var result = TestCommon.RunAICLICommand("list", targetPackageIdentifier);
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);

            // Add the MSIX to simulate an installer doing it
            TestCommon.InstallMsix(TestIndex.MsixInstaller);

            // Install our exe that "installs" the MSIX
            result = TestCommon.RunAICLICommand("install", $"{targetPackageIdentifier} --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            // We should find the package now, and it should be correlated to the MSIX (although we don't actually know that from this probe)
            result = TestCommon.RunAICLICommand("list", targetPackageIdentifier);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            // Remove the MSIX outside of winget's knowledge to keep the tracking data.
            TestCommon.RemoveMsix(Constants.MsixInstallerName);

            // We should not find the package now that the MSIX is gone, confirming that it was correlated
            result = TestCommon.RunAICLICommand("list", targetPackageIdentifier);
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);

            TestCommon.RemoveARPEntry(fakeProductCode);
        }
    }
}
