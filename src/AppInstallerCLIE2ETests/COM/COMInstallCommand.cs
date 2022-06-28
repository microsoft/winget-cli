// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Management.Deployment;
    using NUnit.Framework;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using WinRT;

    public class COMInstallCommand : BaseCommand
    {
        private PackageManager packageManager;
        private string installDir;

        private const string TestPackageCatalog = "TestSource";

        private const string InstallTestMsiInstalledFile = @"AppInstallerTestExeInstaller.exe";
        private const string InstallTestMsiProductId = @"{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        private const string InstallTestMsixName = @"6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        [SetUp]
        public void Init()
        {
            packageManager = new PackageManager();
            installDir = TestCommon.GetRandomTestDir();
        }

        [Test]
        public void InstallAppDoesNotExist()
        {
            var searchResult = FindAllPackages(PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");
            Assert.True(0 == searchResult.Count);
        }

        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            var searchResult = FindAllPackages(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");
            Assert.True(2 == searchResult.Count);
        }

        [Test]
        public async Task InstallExe()
        {
            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
        }

        [Test]
        public async Task InstallExeWithInsufficientMinOsVersion()
        {
            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "InapplicableOsVersion");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.NoApplicableInstallers == installResult.Status);
            Assert.False(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallExeWithHashMismatch()
        {
            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeSha256Mismatch");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.DownloadError == installResult.Status);
            Assert.False(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallWithInno()
        {
            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestInnoInstaller");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallBurn()
        {
            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestBurnInstaller");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallNullSoft()
        {
            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestNullsoftInstaller");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallMSI()
        {
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            var searchResult = FindOnePackage(PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsiInstaller");
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public async Task InstallMSIX()
        {
            // FIXME
            Assert.Ignore();
        }

        [Test]
        public async Task InstallMSIXWithSignature()
        {
            // FIXME
            Assert.Ignore();
        }

        [Test]
        public async Task InstallMSIXWithSignatureHashMismatch()
        {
            // FIXME
            Assert.Ignore();
        }

        [Test]
        public void InstallExeWithAlternateSourceFailure()
        {
            TestCommon.RunAICLICommand("source add", "failSearch \"{ \"\"SearchHR\"\": \"\"0x80070002\"\" }\" Microsoft.Test.Configurable --header \"{}\"");

            var searchResult = FindAllPackages(PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller", "failSearch");
            Assert.True(0 == searchResult.Count);
            ResetTestSource();
        }

        [Test]
        public async Task InstallPortableExe()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false, randomDir: false);
            Assert.True(InstallResultStatus.Ok == installResult.Status);

            // If no location specified, default behavior is to create a package directory with the name "{packageId}_{sourceId}"
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task InstallPortableExeWithCommand()
        {
            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExeWithCommand";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            fileName = "AppInstallerTestExeInstaller.exe";
            commandAlias = "testCommand.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            TestCommon.VerifyPortablePackage(installDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public void InstallPortableExeWithRename()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public void InstallPortableInvalidRename()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public void InstallPortableReservedNames()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public async Task InstallPortableToExistingDirectory()
        {
            var existingDir = Path.Combine(installDir, "testDirectory");
            Directory.CreateDirectory(existingDir);

            string packageId, commandAlias, fileName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            installDir = existingDir;
            var installResult = await Install(searchResult.CatalogPackage, silent: false);
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            TestCommon.VerifyPortablePackage(existingDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task InstallPortableFailsWithCleanup()
        {
            // TODO: To follow up
            Assert.Ignore();

            string winGetDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet");
            string installDir = Path.Combine(winGetDir, "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            // Create a directory with the same name as the symlink in order to cause install to fail.
            string symlinkDirectory = Path.Combine(winGetDir, "Links");
            string conflictDirectory = Path.Combine(symlinkDirectory, commandAlias);
            Directory.CreateDirectory(conflictDirectory);

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false, randomDir: false);

            // Remove directory prior to assertions as this will impact other tests if assertions fail.
            Directory.Delete(conflictDirectory, true);

            Assert.AreNotEqual(InstallResultStatus.Ok, installResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        public IReadOnlyList<MatchResult> FindAllPackages(
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value,
            string source = TestPackageCatalog)
        {
            var findPackageOptions = new FindPackagesOptions();
            findPackageOptions.Filters.Add(new()
            {
                Field = field,
                Option = option,
                Value = value
            });

            var testSource = packageManager.GetPackageCatalogByName(source);
            Assert.NotNull(testSource, $"Ensure that {source} is added");
            var owcSource = testSource.Connect().PackageCatalog;
            return owcSource.FindPackages(findPackageOptions).Matches;
        }

        public MatchResult FindOnePackage(
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value,
            string source = TestPackageCatalog)
        {
            var findPackages = FindAllPackages(field, option, value, source);
            Assert.True(1 == findPackages.Count);
            return findPackages.First();
        }

        public async Task<InstallResult> Install(CatalogPackage package, bool silent = true, bool randomDir = true)
        {
            // Configure install options
            var installOptions = new InstallOptions();
            if (silent)
            {
                installOptions.PackageInstallMode = PackageInstallMode.Silent;
            }

            if (randomDir)
            {
                installOptions.PreferredInstallLocation = installDir;
            }

            return await packageManager.InstallPackageAsync(package, installOptions);
        }

        // TODO Reuse from InstallCommand
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