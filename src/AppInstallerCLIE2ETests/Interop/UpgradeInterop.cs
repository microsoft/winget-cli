// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Win32;
    using NUnit.Framework;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;

    public class UpgradeInterop : BaseCommand
    {
        private PackageManager packageManager;
        private PackageCatalogReference packageCatalogReference;

        private const string TestPackageCatalog = "TestSource";

        private const string UninstallSubKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall";
        private const string WinGetPackageIdentifier = "WinGetPackageIdentifier";
        private const string WinGetSourceIdentifier = "WinGetSourceIdentifier";

        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            ConfigureFeature("portableInstall", true);
        }

        [SetUp]
        public void Init()
        {
            packageManager = new PackageManager();

            var testPackageCatalogReference = packageManager.GetPackageCatalogByName(TestPackageCatalog);
            Assert.NotNull(testPackageCatalogReference, $"Ensure that {TestPackageCatalog} is added");
            
            var options = new CreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testPackageCatalogReference);
            options.CompositeSearchBehavior = CompositeSearchBehavior.RemotePackagesFromAllCatalogs;
            packageCatalogReference = packageManager.CreateCompositePackageCatalog(options);
        }   

        [Test]
        public async Task UpgradePortable()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            try
            {
                var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.Null(searchResult.CatalogPackage.InstalledVersion);

                var installResult = await Install(searchResult.CatalogPackage, "1.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

                searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

                var upgradeResult = await Upgrade(searchResult.CatalogPackage, "2.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);
            }
            finally
            {
                TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            }
        }

        [Test]
        public async Task UpgradePortableARPMismatch()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            try
            {
                var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.Null(searchResult.CatalogPackage.InstalledVersion);

                var installResult = await Install(searchResult.CatalogPackage, "1.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

                // Modify packageId to cause mismatch.
                ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");

                searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

                var upgradeResult = await Upgrade(searchResult.CatalogPackage, "2.0.0.0");
                Assert.AreEqual(InstallResultStatus.InstallError, upgradeResult.Status);
                Assert.AreEqual(Constants.ErrorCode.ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, (int)upgradeResult.InstallerErrorCode);
            }
            finally
            {
                TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            }
        }

        [Test]
        public async Task UpgradePortableForcedOverride()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            try
            {
                var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.Null(searchResult.CatalogPackage.InstalledVersion);

                var installResult = await Install(searchResult.CatalogPackage, "1.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

                // Modify packageId and sourceId to cause mismatch.
                ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");
                ModifyPortableARPEntryValue(productCode, WinGetSourceIdentifier, "testSourceId");

                searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

                var upgradeResult = await Upgrade(searchResult.CatalogPackage, "2.0.0.0", force: true);
                Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);
            }
            finally
            {
                TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            }
        }

        [Test]
        public async Task UpgradePortableUninstallPrevious()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            try
            {
                var searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.Null(searchResult.CatalogPackage.InstalledVersion);

                var installResult = await Install(searchResult.CatalogPackage, "1.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

                searchResult = FindOnePackage(PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
                Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

                var upgradeResult = await Upgrade(searchResult.CatalogPackage, "3.0.0.0");
                Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);
            }
            finally
            {
                TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
            }
        }

        private void ModifyPortableARPEntryValue(string productCode, string name, string value)
        {
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(UninstallSubKey, true))
            {
                RegistryKey entry = uninstallRegistryKey.OpenSubKey(productCode, true);
                entry.SetValue(name, value);
            }
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

        public async Task<InstallResult> Install(CatalogPackage package, string version)
        {
            // Configure install options
            var installOptions = new InstallOptions();
            installOptions.PackageVersionId = package.AvailableVersions.First(i => i.Version == version);

            return await packageManager.InstallPackageAsync(package, installOptions);
        }

        public async Task<InstallResult> Upgrade(CatalogPackage package, string version, bool force = false)
        {
            // Configure install options
            var installOptions = new InstallOptions();
            installOptions.AllowHashMismatch = force;
            installOptions.PackageVersionId = package.AvailableVersions.First(i => i.Version == version);

            return await packageManager.UpgradePackageAsync(package, installOptions);
        }
    }
}
