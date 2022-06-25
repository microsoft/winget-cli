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

    public class COMUninstallCommand : BaseCommand
    {
        // Custom product code for overriding the default in the test exe
        private const string CustomProductCode = "{f08fc03c-0b7e-4fca-9b3c-3a384d18a9f3}";

        // File written when uninstalling the test exe
        private const string UninstallTestExeUninstalledFile = "TestExeUninstalled.txt";

        // Name of a file installed by the MSI that will be removed during uninstall
        private const string UninstallTestMsiInstalledFile = "AppInstallerTestExeInstaller.exe";

        // Package name of the test MSIX package
        private const string UninstallTestMsixName = "6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e";

        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference packageCatalogReference;

        private const string TestPackageCatalog = "TestSource";

        [SetUp]
        public void Init()
        {
            packageManager = new PackageManager();
            installDir = TestCommon.GetRandomTestDir();

            var testPackageCatalogReference = packageManager.GetPackageCatalogByName(TestPackageCatalog);
            Assert.NotNull(testPackageCatalogReference, $"Ensure that {TestPackageCatalog} is added");

            var options = new CreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testPackageCatalogReference);
            options.CompositeSearchBehavior = CompositeSearchBehavior.RemotePackagesFromAllCatalogs;
            packageCatalogReference = packageManager.CreateCompositePackageCatalog(options);
        }   

        [Test]
        public async Task UninstallTestExe()
        {
            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);
            var installResult = await Install(searchResult.CatalogPackage);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);
            var uninstallResult = await Uninstall(searchResult.CatalogPackage);
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(VerifyTestExeUninstalled(installDir));
        }

        [Test]
        public void UninstallTestMsi()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public async Task UninstallTestMsix()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public async Task UninstallPortable()
        {
            // Uninstall a Portable
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false, randomDir: false);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var uninstallResult = await Uninstall(searchResult.CatalogPackage);
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        [Test]
        public async Task UninstallPortableWithProductCode()
        {
            // Uninstall a Portable with ProductCode
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false, randomDir: false);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            searchResult = FindOnePackage(PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, productCode);
            var uninstallResult = await Uninstall(searchResult.CatalogPackage);
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        [Test]
        public async Task UninstallPortableModifiedSymlink()
        {
            string packageId, commandAlias;  
            packageId = "AppInstallerTest.TestPortableExe";
            commandAlias = "AppInstallerTestExeInstaller.exe";

            var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var installResult = await Install(searchResult.CatalogPackage, silent: false);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            string symlinkDirectory = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Links");
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Replace symlink with modified symlink
            File.Delete(symlinkPath);
            FileSystemInfo modifiedSymlinkInfo = File.CreateSymbolicLink(symlinkPath, "fakeTargetExe");

            searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            var uninstallResult = await Uninstall(searchResult.CatalogPackage);
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);

            // Remove modified symlink as to not interfere with other tests
            bool modifiedSymlinkExists = modifiedSymlinkInfo.Exists;
            modifiedSymlinkInfo.Delete();

            Assert.True(modifiedSymlinkExists, "Modified symlink should still exist");
        }

        [Test]
        public async Task UninstallNotIndexed()
        {
            // TODO
            Assert.Ignore();
        }

        [Test]
        public void UninstallAppNotInstalled()
        {
            // TODO
            Assert.Ignore();
        }

        private bool VerifyTestExeUninstalled(string installDir)
        {
            return File.Exists(Path.Combine(installDir, UninstallTestExeUninstalledFile));
        }

        private bool VerifyTestMsiUninstalled(string installDir)
        {
            return !File.Exists(Path.Combine(installDir, UninstallTestMsiInstalledFile));
        }

        private bool VerifyTestMsixUninstalled()
        {
            var result = TestCommon.RunCommandWithResult("powershell", $"Get-AppxPackage {UninstallTestMsixName}");
            return string.IsNullOrWhiteSpace(result.StdOut);
        }

        public IReadOnlyList<MatchResult> FindAllPackages(
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            var findPackageOptions = new FindPackagesOptions();
            findPackageOptions.Filters.Add(new()
            {
                Field = field,
                Option = option,
                Value = value
            });

            var owcSource = packageCatalogReference.Connect().PackageCatalog;
            return owcSource.FindPackages(findPackageOptions).Matches;
        }

        public MatchResult FindOnePackage(
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            var findPackages = FindAllPackages(field, option, value);
            Assert.True(1 == findPackages.Count);
            return findPackages.First();
        }

        public async Task<InstallResult> Install(CatalogPackage package, bool silent = true, bool randomDir = true, string overrideArg = null)
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

            if (overrideArg != null)
            {
                installOptions.ReplacementInstallerArguments = overrideArg;
            }

            return await packageManager.InstallPackageAsync(package, installOptions);
        }

        public async Task<UninstallResult> Uninstall(CatalogPackage package)
        {
            var uninstallOptions = new UninstallOptions();
            return await packageManager.UninstallPackageAsync(package, uninstallOptions);
        }
    }
}
