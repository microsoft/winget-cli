// -----------------------------------------------------------------------------
// <copyright file="RepairInterop.cs" company="Microsoft Corporation">
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
    /// Repair Interop Tests for COM/WinRT Interop classes.
    /// </summary>
    ////[TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class RepairInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="RepairInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public RepairInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Test setup.
        /// </summary>
        [SetUp]
        public void Init()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
            this.installDir = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());

            // Create a composite source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            this.compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
        }

        /// <summary>
        /// Test Repair MSI Installer.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test. </returns>
        [Test]
        public async Task RepairMSIInstaller()
        {
            // Find a package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMsiRepair");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install the package
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should be detected as installed
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMsiRepair");
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Note: The 'msiexec repair' command requires the original installer file to be present at the location registered in the ARP (Add/Remove Programs).
            // In our test scenario, the MSI installer file is initially placed in a temporary location and then deleted, which can cause the repair operation to fail.
            // To work around this, we copy the installer file to the ARP source directory before running the repair command.
            // A more permanent solution would be to modify the MSI installer to cache the installer file in a known location and register that location as the installer source.
            // This would allow the 'msiexec repair' command to function as expected.
            string installerSourceDir = TestCommon.CopyInstallerFileToARPInstallSourceDirectory(TestCommon.GetTestDataFile("AppInstallerTestMsiInstallerV2.msi"), Constants.MsiInstallerProductCode, true);

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;

            //// We need to pass argument to indicate silent repair - this should be added to the RepairOptions class.
            //// repairOptions.RepairString  - This should be added to the RepairOptions class.

            var repairResult = await this.packageManager.RepairPackageAsync(searchResult.CatalogPackage, repairOptions);
            Assert.AreEqual(RepairResultStatus.Ok, repairResult.Status);

            // Uninstall the package
            var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(this.installDir));

            if (installerSourceDir != null && Directory.Exists(installerSourceDir))
            {
                Directory.Delete(installerSourceDir, true);
            }
        }
    }
}
