// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading.Tasks;

    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class UpgradeInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        public UpgradeInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void Init()
        {
            packageManager = TestFactory.CreatePackageManager();

            // Create composite package catalog source
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            compositeSource = packageManager.CreateCompositePackageCatalog(options);
        }

        [Test]
        public async Task UpgradePortable()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageId = Constants.PortableExePackageId;
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "1.0.0.0"));

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");

            // Configure upgrade options
            var upgradeOptions = TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "2.0.0.0"));

            // Upgrade
            var upgradeResult = await packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "2.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task UpgradePortableARPMismatch()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageId = Constants.PortableExePackageId;
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "1.0.0.0"));

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Modify packageId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Configure upgrade options
            var upgradeOptions = TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "2.0.0.0"));

            // Upgrade
            var upgradeResult = await packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.InstallError, upgradeResult.Status);
            Assert.AreEqual(Constants.ErrorCode.ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, upgradeResult.ExtendedErrorCode.HResult);

            // Find package again, it should have not been upgraded
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task UpgradePortableForcedOverride()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageId = Constants.PortableExePackageId;
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "1.0.0.0"));

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Modify packageId and sourceId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetSourceIdentifier, "testSourceId");

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Configure upgrade options
            var upgradeOptions = TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "2.0.0.0"));
            upgradeOptions.AllowHashMismatch = true;

            // Upgrade
            var upgradeResult = await packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "2.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task UpgradePortableUninstallPrevious()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageId = Constants.PortableExePackageId;
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "1.0.0.0"));

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");

            // Configure upgrade options
            var upgradeOptions = TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, (i => i.Version == "3.0.0.0"));

            // Upgrade
            var upgradeResult = await packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "3.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        // Cannot use foreach or Linq for out-of-process IVector
        // Bug: https://github.com/microsoft/CsWinRT/issues/1205
        public static T First<T>(IReadOnlyList<T> list, Func<T, bool> condition)
        {
            if (list == null || condition == null)
            {
                throw new ArgumentNullException();
            }

            for (int i = 0; i < list.Count; ++i)
            {
                var item = list[i];
                if (condition(item))
                {
                    return item;
                }
            }

            throw new InvalidOperationException("No element satisfies the condition");
        }
    }
}
