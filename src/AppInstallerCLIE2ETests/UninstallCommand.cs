// -----------------------------------------------------------------------------
// <copyright file="UninstallCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test uninstall command.
    /// </summary>
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

        /// <summary>
        /// Test uninstall exe.
        /// </summary>
        [Test]
        public void UninstallTestExe()
        {
            // Uninstall an Exe
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.ExeInstallerPackageId} --silent -l {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", Constants.ExeInstallerPackageId);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            Assert.That(TestCommon.VerifyTestExeUninstalled(installDir), Is.True);
        }

        /// <summary>
        /// Test uninstall msi.
        /// </summary>
        [Test]
        public void UninstallTestMsi()
        {
            if (string.IsNullOrEmpty(TestIndex.MsiInstaller))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Uninstall an MSI
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.MsiInstallerPackageId} -l {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", Constants.MsiInstallerPackageId);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            Assert.That(TestCommon.VerifyTestMsiUninstalled(installDir), Is.True);
        }

        /// <summary>
        /// Test uninstall msix.
        /// </summary>
        [Test]
        public void UninstallTestMsix()
        {
            // Uninstall an MSIX
            TestCommon.RunAICLICommand("install", Constants.MsixInstallerPackageId);
            var result = TestCommon.RunAICLICommand("uninstall", Constants.MsixInstallerPackageId);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            Assert.That(TestCommon.VerifyTestMsixUninstalled(), Is.True);
        }

        /// <summary>
        /// Test uninstall msix package with machine scope.
        /// </summary>
        [Test]
        public void UninstallTestMsixMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            // Uninstall an MSIX
            TestCommon.RunAICLICommand("install", $"{Constants.MsixInstallerPackageId} --scope machine");
            var result = TestCommon.RunAICLICommand("uninstall", $"{Constants.MsixInstallerPackageId} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            Assert.That(TestCommon.VerifyTestMsixUninstalled(true), Is.True);
        }

        /// <summary>
        /// Test uninstall portable package.
        /// </summary>
        [Test]
        public void UninstallPortable()
        {
            // Uninstall a Portable
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            TestCommon.RunAICLICommand("install", $"{packageId}");
            var result = TestCommon.RunAICLICommand("uninstall", $"{packageId}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall portable package with product code.
        /// </summary>
        [Test]
        public void UninstallPortableWithProductCode()
        {
            // Uninstall a Portable with ProductCode
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            TestCommon.RunAICLICommand("install", $"{packageId}");
            var result = TestCommon.RunAICLICommand("uninstall", $"--product-code {productCode}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall portable package with modified symlink.
        /// </summary>
        [Test]
        public void UninstallPortableModifiedSymlink()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            TestCommon.RunAICLICommand("install", $"{packageId}");

            string symlinkDirectory = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Replace symlink with modified symlink
            File.Delete(symlinkPath);
            FileSystemInfo modifiedSymlinkInfo = File.CreateSymbolicLink(symlinkPath, "fakeTargetExe");

            var result = TestCommon.RunAICLICommand("uninstall", $"{packageId}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_PORTABLE_UNINSTALL_FAILED));
            Assert.That(result.StdOut, Does.Contain("Unable to remove Portable package as it has been modified; to override this check use --force"));
            Assert.That(modifiedSymlinkInfo.Exists, Is.True, "Modified symlink should still exist");

            // Try again with --force
            var result2 = TestCommon.RunAICLICommand("uninstall", $"{packageId} --force");
            Assert.That(result2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result2.StdOut, Does.Contain("Portable package has been modified; proceeding due to --force"));
            Assert.That(result2.StdOut, Does.Contain("Successfully uninstalled"));

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall zip portable package.
        /// </summary>
        [Test]
        public void UninstallZip_Portable()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestZipInstallerWithPortable";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = "TestPortable.exe";
            fileName = "AppInstallerTestExeInstaller.exe";

            var testResult = TestCommon.RunAICLICommand("install", $"{packageId}");
            var result = TestCommon.RunAICLICommand("uninstall", $"{packageId}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall not indexed.
        /// </summary>
        [Test]
        public void UninstallNotIndexed()
        {
            // Uninstalls a package found with ARP not matching any known manifest.
            // Install the test EXE providing a custom Product Code so that it cannot be mapped
            // back to its manifest, then uninstall it using its Product Code
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"{Constants.ExeInstallerPackageId} --override \"/ProductID {CustomProductCode} /InstallDir {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", CustomProductCode);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully uninstalled"));
            Assert.That(TestCommon.VerifyTestExeUninstalled(installDir), Is.True);
        }

        /// <summary>
        /// Test uninstalled app not found.
        /// </summary>
        [Test]
        public void UninstallAppNotInstalled()
        {
            // Verify failure when trying to uninstall an app that is not installed.
            var result = TestCommon.RunAICLICommand("uninstall", $"TestMsixInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("No installed package found matching input criteria."));
        }
    }
}
