// -----------------------------------------------------------------------------
// <copyright file="UninstallInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Test uninstall interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class UninstallInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public UninstallInterop(IInstanceInitializer initializer)
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
            this.installDir = TestCommon.GetRandomTestDir();

            // Create composite package catalog source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            this.compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
        }

        /// <summary>
        /// Test uninstall exe.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallTestExe()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestExeUninstalled(this.installDir));
        }

        /// <summary>
        /// Test uninstall msi.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallTestMsi()
        {
            if (string.IsNullOrEmpty(TestIndex.MsiInstaller))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsiInstallerPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsiInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsiUninstalled(this.installDir));
        }

        /// <summary>
        /// Test uninstall msix.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallTestMsix()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsixUninstalled());
        }

        /// <summary>
        /// Test uninstall msix with machine scope.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallTestMsixMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallScope = PackageInstallScope.System;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallOptions = this.TestFactory.CreateUninstallOptions();
            uninstallOptions.PackageUninstallScope = PackageUninstallScope.System;

            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, uninstallOptions);
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsixUninstalled(true));
        }

        /// <summary>
        /// Test uninstall portable package.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallPortable()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall portable package with product code.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallPortableWithProductCode()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, productCode);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test uninstall portable package modified symlink.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallPortableModifiedSymlink()
        {
            string packageId = Constants.PortableExePackageId;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string symlinkDirectory = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Links");
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Replace symlink with modified symlink
            File.Delete(symlinkPath);
            FileSystemInfo modifiedSymlinkInfo = File.CreateSymbolicLink(symlinkPath, "fakeTargetExe");

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.UninstallError, uninstallResult.Status);
            Assert.True(modifiedSymlinkInfo.Exists, "Modified symlink should still exist");

            // Remove modified symlink as to not interfere with other tests
            modifiedSymlinkInfo.Delete();
        }

        /// <summary>
        /// Test uninstall not indexed.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task UninstallNotIndexed()
        {
            const string customProductCode = "{f08fc03c-0b7e-4fca-9b3c-3a384d18a9f3}";

            // Find package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ExeInstallerPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/ProductID {customProductCode} /InstallDir {this.installDir}";

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, customProductCode);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Uninstall
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestExeUninstalled(this.installDir));
        }
    }
}
