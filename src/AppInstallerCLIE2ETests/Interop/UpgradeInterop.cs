// -----------------------------------------------------------------------------
// <copyright file="UpgradeInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Test upgrade interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class UpgradeInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="UpgradeInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public UpgradeInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Init()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();

            // Create composite package catalog source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            this.compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
        }

        /// <summary>
        /// Tests upgrade portable package.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
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
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "1.0.0.0");

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");

            // Configure upgrade options
            var upgradeOptions = this.TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "2.0.0.0");

            // Upgrade
            var upgradeResult = await this.packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "2.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package with arp mismatch.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
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
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "1.0.0.0");

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Modify packageId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Configure upgrade options
            var upgradeOptions = this.TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "2.0.0.0");

            // Upgrade
            var upgradeResult = await this.packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.InstallError, upgradeResult.Status);
            Assert.AreEqual(Constants.ErrorCode.ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, upgradeResult.ExtendedErrorCode.HResult);

            // Find package again, it should have not been upgraded
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package force.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
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
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "1.0.0.0");

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Modify packageId and sourceId to cause mismatch.
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetPackageIdentifier, "testPackageId");
            TestCommon.ModifyPortableARPEntryValue(productCode, Constants.WinGetSourceIdentifier, "testSourceId");

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Configure upgrade options
            var upgradeOptions = this.TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "2.0.0.0");
            upgradeOptions.Force = true;

            // Upgrade
            var upgradeResult = await this.packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "2.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test upgrade portable package uninstall previous.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
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
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "1.0.0.0");

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");

            // Configure upgrade options
            var upgradeOptions = this.TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "3.0.0.0");

            // Upgrade
            var upgradeResult = await this.packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Find package again, but this time it should detect the upgraded installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "3.0.0.0");
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        // Cannot use foreach or Linq for out-of-process IVector
        // Bug: https://github.com/microsoft/CsWinRT/issues/1205
        private static T First<T>(IReadOnlyList<T> list, Func<T, bool> condition)
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
