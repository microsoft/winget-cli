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
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("sourcePriority", true);
        }

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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("No package found matching input criteria."));
        }

        /// <summary>
        /// Test multiple matches found.
        /// </summary>
        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            var result = TestCommon.RunAICLICommand("install", "TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("Multiple packages found matching input criteria. Please refine the input."));
        }

        /// <summary>
        /// Test install exe.
        /// </summary>
        [Test]
        public void InstallExe()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.False);
        }

        /// <summary>
        /// Test install exe hash mismatch.
        /// </summary>
        [Test]
        public void InstallExeWithHashMismatch()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestExeSha256Mismatch --silent -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH));
            Assert.That(result.StdOut, Does.Contain("Installer hash does not match"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.False);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/VERYSILENT"), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/quiet"), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/S"), Is.True);
        }

        /// <summary>
        /// Test install msi.
        /// </summary>
        [Test]
        public void InstallMSI()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir), Is.True);
        }

        /// <summary>
        /// Test install msix.
        /// </summary>
        [Test]
        public void InstallMSIX()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(true), Is.True);
        }

        /// <summary>
        /// Test install msix with signature hash.
        /// </summary>
        [Test]
        public void InstallMSIXWithSignature()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixWithSignatureHash");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(true), Is.True);
        }

        /// <summary>
        /// Test msix hash mismatch.
        /// </summary>
        [Test]
        public void InstallMSIXWithSignatureHashMismatch()
        {
            var result = TestCommon.RunAICLICommand("install", $"TestMsixSignatureHashMismatch");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH));
            Assert.That(result.StdOut, Does.Contain("Installer hash does not match"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(), Is.False);
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
                Assert.That(result.ExitCode, Is.EqualTo(unchecked((int)0x80070002)));
                Assert.That(result.StdOut, Does.Contain("Failed when searching source: failSearch"));
                Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExeInstaller"));
                Assert.That(result.StdOut, Does.Not.Contain("Successfully installed"));
                Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.False);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));

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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.Not.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("The specified filename is not a valid filename"));
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
            Assert.That(result.ExitCode, Is.Not.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("The specified filename is not a valid filename"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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

            Assert.That(result.ExitCode, Is.Not.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Unable to create symlink"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            string symlinkDirectory = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Clean first install should not display file overwrite message.
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(result.StdOut, Does.Not.Contain($"Overwriting existing file: {symlinkPath}"));

            // Perform second install and verify that file overwrite message is displayed.
            var result2 = TestCommon.RunAICLICommand("install", $"{packageId} --force");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Successfully installed"));

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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true, TestCommon.Scope.User, true);
        }

        /// <summary>
        /// Test install portable with rename creates hardlink instead of renaming original.
        /// </summary>
        [Test]
        public void InstallPortableWithRename_VerifyHardlink()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            string renameArgValue = "customAlias.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} --rename {renameArgValue}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            string installPath = Path.Combine(installDir, packageDirName);
            string originalFile = Path.Combine(installPath, "AppInstallerTestExeInstaller.exe");
            string hardlinkFile = Path.Combine(installPath, renameArgValue);

            // Verify original file exists with original name (not renamed)
            Assert.True(File.Exists(originalFile), $"Original file should exist at: {originalFile}");

            // Verify hardlink exists
            Assert.True(File.Exists(hardlinkFile), $"Hardlink should exist at: {hardlinkFile}");

            // Verify hardlink and original point to same content (equivalence)
            byte[] originalBytes = File.ReadAllBytes(originalFile);
            byte[] hardlinkBytes = File.ReadAllBytes(hardlinkFile);
            Assert.AreEqual(originalBytes.Length, hardlinkBytes.Length, "File sizes should be equal");
            Assert.True(originalBytes.AsSpan().SequenceEqual(hardlinkBytes.AsSpan()), "Hardlink should be equivalent to original file");

            // Verify uninstall removes both original and hardlink
            var uninstallResult = TestCommon.RunAICLICommand("uninstall", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode);
            Assert.False(File.Exists(originalFile), $"Original file should be removed after uninstall");
            Assert.False(File.Exists(hardlinkFile), $"Hardlink should be removed after uninstall");
        }

        /// <summary>
        /// Test install portable with Commands field creates hardlinks for all command aliases.
        /// </summary>
        [Test]
        public void InstallPortableWithCommands_VerifyHardlinks()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            string commandAlias = "testCommand.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            string installPath = Path.Combine(installDir, packageDirName);
            string originalFile = Path.Combine(installPath, "AppInstallerTestExeInstaller.exe");
            string hardlinkFile = Path.Combine(installPath, commandAlias);

            // Verify original file exists with original name
            Assert.True(File.Exists(originalFile), $"Original file should exist at: {originalFile}");

            // Verify command alias hardlink exists
            Assert.True(File.Exists(hardlinkFile), $"Command alias hardlink should exist at: {hardlinkFile}");

            // Verify hardlink is equivalent to original
            byte[] originalBytes = File.ReadAllBytes(originalFile);
            byte[] hardlinkBytes = File.ReadAllBytes(hardlinkFile);
            Assert.AreEqual(originalBytes.Length, hardlinkBytes.Length, "File sizes should be equal");
            Assert.True(originalBytes.AsSpan().SequenceEqual(hardlinkBytes.AsSpan()), "Command alias hardlink should be equivalent to original file");

            // Cleanup
            TestCommon.RunAICLICommand("uninstall", $"{packageId}");
        }

        /// <summary>
        /// Test install zip portable with PortableCommandAlias creates hardlinks for nested files.
        /// </summary>
        [Test]
        public void InstallZip_PortableWithCommandAlias_VerifyHardlinks()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, packageDirName, productCode;
            packageId = "AppInstallerTest.TestZipInstallerWithPortable";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            string originalFileName = "AppInstallerTestExeInstaller.exe";
            string commandAlias = "TestPortable.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            string installPath = Path.Combine(installDir, packageDirName);
            string originalFile = Path.Combine(installPath, originalFileName);
            string hardlinkFile = Path.Combine(installPath, commandAlias);

            // Verify original extracted file exists with original name
            Assert.True(File.Exists(originalFile), $"Original extracted file should exist at: {originalFile}");

            // Verify hardlink for command alias exists
            Assert.True(File.Exists(hardlinkFile), $"Command alias hardlink should exist at: {hardlinkFile}");

            // Verify hardlink is equivalent to original
            byte[] originalBytes = File.ReadAllBytes(originalFile);
            byte[] hardlinkBytes = File.ReadAllBytes(hardlinkFile);
            Assert.AreEqual(originalBytes.Length, hardlinkBytes.Length, "File sizes should be equal");
            Assert.True(originalBytes.AsSpan().SequenceEqual(hardlinkBytes.AsSpan()), "Archive portable hardlink should be equivalent to original file");

            // Cleanup
            TestCommon.RunAICLICommand("uninstall", $"{packageId}");
        }

        /// <summary>
        /// Test install zip with invalid relative file path.
        /// </summary>
        [Test]
        public void InstallZipWithInvalidRelativeFilePath()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInvalidRelativePath");
            Assert.That(result.ExitCode, Is.Not.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Invalid relative file path to the nested installer; path points to a location outside of the install directory"));
        }

        /// <summary>
        /// Test install zip msi.
        /// </summary>
        [Test]
        public void InstallZipWithMsi()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsi --silent -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir), Is.True);
        }

        /// <summary>
        /// Test install zip msix.
        /// </summary>
        [Test]
        public void InstallZipWithMsix()
        {
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestZipInstallerWithMsix");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestMsixInstalledAndCleanup(), Is.True);
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);
        }

        /// <summary>
        /// Test install an installed package and convert to upgrade.
        /// </summary>
        [Test]
        public void InstallExeFoundExistingConvertToUpgrade()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 1.0.0.0 --silent -l {baseDir}");
            Assert.That(baseResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(baseResult.StdOut, Does.Contain("Successfully installed"));

            // Install will convert to upgrade
            var upgradeDir = TestCommon.GetRandomTestDir();
            var upgradeResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {upgradeDir}");
            Assert.That(upgradeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(upgradeResult.StdOut, Does.Contain("Trying to upgrade the installed package..."));
            Assert.That(upgradeResult.StdOut, Does.Contain("Successfully installed"));

            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(upgradeDir, "/Version 2.0.0.0"), Is.True);
        }

        /// <summary>
        /// Test install an installed package without an available upgrade.
        /// </summary>
        [Test]
        public void InstallExeFoundExistingConvertToUpgradeNoAvailableUpgrade()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 2.0.0.0 --silent -l {baseDir}");
            Assert.That(baseResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(baseResult.StdOut, Does.Contain("Successfully installed"));

            // Install will convert to upgrade
            var upgradeDir = TestCommon.GetRandomTestDir();
            var upgradeResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {upgradeDir}");
            Assert.That(upgradeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_UPDATE_NOT_APPLICABLE));
            Assert.That(upgradeResult.StdOut, Does.Contain("Trying to upgrade the installed package..."));
            Assert.That(upgradeResult.StdOut, Does.Contain("No available upgrade"));

            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir), Is.True);
        }

        /// <summary>
        /// Test force installing a package.
        /// </summary>
        [Test]
        public void InstallExeWithLatestInstalledWithForce()
        {
            var baseDir = TestCommon.GetRandomTestDir();
            var baseResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 2.0.0.0 --silent -l {baseDir}");
            Assert.That(baseResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(baseResult.StdOut, Does.Contain("Successfully installed"));

            // Install will not convert to upgrade
            var installDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -v 1.0.0.0 --silent -l {installDir} --force");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(baseDir), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);
        }

        /// <summary>
        /// Test install a package with an invalid Windows Feature dependency.
        /// </summary>
        [Test]
        public void InstallWithWindowsFeatureDependency_FeatureNotFound()
        {
            var testDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.WindowsFeature -l {testDir}");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALL_DEPENDENCIES));
            Assert.That(installResult.StdOut, Does.Contain("The feature [invalidFeature] was not found."));
            Assert.That(installResult.StdOut, Does.Contain("Failed to enable Windows Feature dependencies. To proceed with installation, use '--force'."));
        }

        /// <summary>
        /// Test install a package with a Windows Feature dependency using the force argument.
        /// </summary>
        [Test]
        public void InstallWithWindowsFeatureDependency_Force()
        {
            var testDir = TestCommon.GetRandomTestDir();
            var installResult = TestCommon.RunAICLICommand("install", $"AppInstallerTest.WindowsFeature --silent --force -l {testDir}");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Failed to enable Windows Feature dependencies; proceeding due to --force"));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(testDir), Is.True);
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
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

            // Portable package is used as a dependency. Ensure that it is installed and cleaned up successfully.
            string portablePackageId, commandAlias, fileName, packageDirName, productCode;
            portablePackageId = "AppInstallerTest.TestPortableExeWithCommand";
            packageDirName = productCode = portablePackageId + "_" + Constants.TestSourceIdentifier;
            fileName = "AppInstallerTestExeInstaller.exe";
            commandAlias = "testCommand.exe";

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(testDir), Is.True);
        }

        /// <summary>
        /// Test install a package with a package dependency and specify dependencies only.
        /// </summary>
        [Test]
        public void InstallWithPackageDependency_DependenciesOnly()
        {
            var testDir = TestCommon.GetRandomTestDir();
            string installDir = TestCommon.GetPortablePackagesDirectory();
            var installResult = TestCommon.RunAICLICommand("install", $"-q AppInstallerTest.PackageDependencyRequiresPathRefresh -l {testDir} --dependencies-only");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Installing dependencies only. The package itself will not be installed."));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

            // Portable package is used as a dependency. Ensure that it is installed and cleaned up successfully.
            string portablePackageId, commandAlias, fileName, packageDirName, productCode;
            portablePackageId = "AppInstallerTest.TestPortableExeWithCommand";
            packageDirName = productCode = portablePackageId + "_" + Constants.TestSourceIdentifier;
            fileName = "AppInstallerTestExeInstaller.exe";
            commandAlias = "testCommand.exe";

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(testDir), Is.False);
        }

        /// <summary>
        /// Test install a package using a specific installer type.
        /// </summary>
        [Test]
        public void InstallWithInstallerTypeArgument()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMultipleInstallers --silent -l {installDir} --installer-type exe");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);
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

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.True, "/S");
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

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICABLE_INSTALLER));
            Assert.That(result.StdOut, Does.Contain("No applicable installer found; see logs for more details."));
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));

            // Add the MSIX to simulate an installer doing it
            TestCommon.InstallMsix(TestIndex.MsixInstaller);

            // Install our exe that "installs" the MSIX
            result = TestCommon.RunAICLICommand("install", $"{targetPackageIdentifier} --force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // We should find the package now, and it should be correlated to the MSIX (although we don't actually know that from this probe)
            result = TestCommon.RunAICLICommand("list", targetPackageIdentifier);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // Remove the MSIX outside of winget's knowledge to keep the tracking data.
            TestCommon.RemoveMsix(Constants.MsixInstallerName);

            // We should not find the package now that the MSIX is gone, confirming that it was correlated
            result = TestCommon.RunAICLICommand("list", targetPackageIdentifier);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));

            TestCommon.RemoveARPEntry(fakeProductCode);
        }

        /// <summary>
        /// Test install source priority.
        /// </summary>
        [Test]
        public void InstallExeWithSourcePriority()
        {
            // This test source always returns a single package from search
            TestCommon.RunAICLICommand("source add", "dummyPackage \"{ \"\"ContainsPackage\"\": true }\" Microsoft.Test.Configurable --header \"{}\"");

            try
            {
                // Attempt install with equal (default) priorities
                var installDir = TestCommon.GetRandomTestDir();
                var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
                Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND));
                Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.False);

                // Change the priority of the primary test source to be higher
                TestCommon.RunAICLICommand("source edit", $"{Constants.TestSourceName} --priority 1");

                result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
                Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExeInstaller"));
                Assert.That(result.StdOut, Does.Contain("Successfully installed"));
                Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.True);
            }
            finally
            {
                this.ResetTestSource();
            }
        }
    }
}
