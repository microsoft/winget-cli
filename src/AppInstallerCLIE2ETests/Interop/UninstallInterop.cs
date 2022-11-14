// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System;
    using System.IO;
    using System.Threading.Tasks;

    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class UninstallInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        public UninstallInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void Init()
        {
            packageManager = TestFactory.CreatePackageManager();
            installDir = TestCommon.GetRandomTestDir();

            // Create composite package catalog source
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            compositeSource = packageManager.CreateCompositePackageCatalog(options);
        }

        [Test]
        public async Task UninstallTestExe()
        {
            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestExeUninstalled(installDir));
        }

        [Test]
        public async Task UninstallTestMsi()
        {
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsiInstallerPackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsiInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsiUninstalled(installDir));
        }

        [Test]
        public async Task UninstallTestMsix()
        {
            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsixUninstalled());
        }

        [Test]
        public async Task UninstallPortable()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        [Test]
        public async Task UninstallPortableWithProductCode()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, productCode);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        [Test]
        public async Task UninstallPortableModifiedSymlink()
        {
            string packageId = Constants.PortableExePackageId;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string symlinkDirectory = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Links");
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Replace symlink with modified symlink
            File.Delete(symlinkPath);
            FileSystemInfo modifiedSymlinkInfo = File.CreateSymbolicLink(symlinkPath, "fakeTargetExe");

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.UninstallError, uninstallResult.Status);
            Assert.True(modifiedSymlinkInfo.Exists, "Modified symlink should still exist");

            // Remove modified symlink as to not interfere with other tests
            modifiedSymlinkInfo.Delete();
        }

        [Test]
        public async Task UninstallNotIndexed()
        {
            const string customProductCode = "{f08fc03c-0b7e-4fca-9b3c-3a384d18a9f3}";

            // Find package
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/ProductID {customProductCode} /InstallDir {installDir}";

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = FindOnePackage(compositeSource, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, customProductCode);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestExeUninstalled(installDir));
        }
    }
}
