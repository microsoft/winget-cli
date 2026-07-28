// -----------------------------------------------------------------------------
// <copyright file="GroupPolicyForInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using Microsoft.WinGet.SharedLib.Exceptions;
    using NUnit.Framework;
    using Windows.System;

    /// <summary>
    /// Group Policy Tests for COM/WinRT Interop classes.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class GroupPolicyForInterop : BaseInterop
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GroupPolicyForInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public GroupPolicyForInterop(IInstanceInitializer initializer)
          : base(initializer)
        {
        }

        /// <summary>
        /// Test setup.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Clean up.
        /// </summary>
        [TearDown]
        public void CleanUp()
        {
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Test class Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TestClassTearDown()
        {
            // Restore the tests source if it was removed as the affects subsequent tests.
            TestCommon.SetupTestSource();
        }

        /// <summary>
        /// Validates disabling WinGetPolicy should block COM/WinRT Objects creation (InProcess and OutOfProcess).
        /// </summary>
        [Test]
        public void DisableWinGetPolicy()
        {
            GroupPolicyHelper.EnableWinget.Disable();

            GroupPolicyException packageManagerException = Assert.Throws<GroupPolicyException>((Action)(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); }));
            Assert.That(packageManagerException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(packageManagerException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException findPackagesOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { FindPackagesOptions findPackagesOptions = this.TestFactory.CreateFindPackagesOptions(); }));
            Assert.That(findPackagesOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(findPackagesOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException createCompositePackageCatalogOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = this.TestFactory.CreateCreateCompositePackageCatalogOptions(); }));
            Assert.That(createCompositePackageCatalogOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(createCompositePackageCatalogOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException installOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { InstallOptions installOptions = this.TestFactory.CreateInstallOptions(); }));
            Assert.That(installOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(installOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException uninstallOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { UninstallOptions uninstallOptions = this.TestFactory.CreateUninstallOptions(); }));
            Assert.That(uninstallOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(uninstallOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException downloadOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { DownloadOptions downloadOptions = this.TestFactory.CreateDownloadOptions(); }));
            Assert.That(downloadOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(downloadOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException packageMatchFilterException = Assert.Throws<GroupPolicyException>((Action)(() => { PackageMatchFilter packageMatchFilter = this.TestFactory.CreatePackageMatchFilter(); }));
            Assert.That(packageMatchFilterException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(packageMatchFilterException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException repairOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { RepairOptions repairOptions = this.TestFactory.CreateRepairOptions(); }));
            Assert.That(repairOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(repairOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException addPackageCatalogOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { AddPackageCatalogOptions packageManagerSettings = this.TestFactory.CreateAddPackageCatalogOptions(); }));
            Assert.That(addPackageCatalogOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(addPackageCatalogOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException removePackageCatalogOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { RemovePackageCatalogOptions packageManagerSettings = this.TestFactory.CreateRemovePackageCatalogOptions(); }));
            Assert.That(removePackageCatalogOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(removePackageCatalogOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            GroupPolicyException editPackageCatalogOptionsException = Assert.Throws<GroupPolicyException>((Action)(() => { EditPackageCatalogOptions packageManagerSettings = this.TestFactory.CreateEditPackageCatalogOptions(); }));
            Assert.That(editPackageCatalogOptionsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
            Assert.That(editPackageCatalogOptionsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));

            // PackageManagerSettings is not implemented in context OutOfProcDev
            if (this.TestFactory.Context == ClsidContext.InProc)
            {
                GroupPolicyException packageManagerSettingsException = Assert.Throws<GroupPolicyException>((Action)(() => { PackageManagerSettings packageManagerSettings = this.TestFactory.CreatePackageManagerSettings(); }));
                Assert.That(packageManagerSettingsException.Message, Is.EqualTo(Constants.BlockByWinGetPolicyErrorMessage));
                Assert.That(packageManagerSettingsException.HResult, Is.EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY));
            }
        }

        /// <summary>
        /// Validates disabling the EnableWindowsPackageManagerCommandLineInterfaces policy should still allow COM calls.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DisableWinGetCommandLineInterfacesPolicy()
        {
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.Disable();

            PackageManager packageManager = this.TestFactory.CreatePackageManager();
            string installDir = TestCommon.GetRandomTestDir();

            // Create composite package catalog source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.That(testSource, Is.Not.Null, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            PackageCatalogReference compositeSource = packageManager.CreateCompositePackageCatalog(options);

            // Find package
            var searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ModifyRepairInstaller);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.AcceptPackageAgreements = true;
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /Version 2.0.0.0 /DisplayName TestModifyRepair /UseHKLM";

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.That(installResult.Status, Is.EqualTo(InstallResultStatus.Ok));

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ModifyRepairInstaller);
            Assert.That(searchResult.CatalogPackage.InstalledVersion, Is.Not.Null);

            // Repair
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;
            var repairResult = await packageManager.RepairPackageAsync(searchResult.CatalogPackage, repairOptions);
            Assert.That(repairResult.Status, Is.EqualTo(RepairResultStatus.Ok));

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.That(uninstallResult.Status, Is.EqualTo(UninstallResultStatus.Ok));
            Assert.That(TestCommon.VerifyTestExeUninstalled(installDir), Is.True);

            // Clean up.
            if (Directory.Exists(installDir))
            {
                Directory.Delete(installDir, true);
            }

            // Download
            var downloadResult = await packageManager.DownloadPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateDownloadOptions());
            Assert.That(downloadResult.Status, Is.EqualTo(DownloadResultStatus.Ok));
            var packageVersion = "2.0.0.0";
            string downloadDir = Path.Combine(TestCommon.GetDefaultDownloadDirectory(), $"{Constants.ModifyRepairInstaller}_{packageVersion}");
            TestCommon.AssertInstallerDownload(downloadDir, "TestModifyRepair", packageVersion, ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Burn, "en-US");

            // Add, update and remove package catalog
            await this.AddUpdateRemovePackageCatalog();
        }

        private async Task AddUpdateRemovePackageCatalog()
        {
            // Remove the tests source if it exists.
            await this.RemovePackageCatalog();

            PackageManager packageManager = this.TestFactory.CreatePackageManager();

            // Add package catalog
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            var addCatalogResult = await packageManager.AddPackageCatalogAsync(options);
            Assert.That(addCatalogResult, Is.Not.Null);
            Assert.That(addCatalogResult.Status, Is.EqualTo(AddPackageCatalogStatus.Ok));

            // Get package catalog
            var packageCatalog = packageManager.GetPackageCatalogByName(options.Name);

            Assert.That(packageCatalog, Is.Not.Null);
            Assert.That(packageCatalog.Info.Name, Is.EqualTo(options.Name));
            Assert.That(packageCatalog.Info.Argument, Is.EqualTo(options.SourceUri));
            var lastUpdatedTime = packageCatalog.Info.LastUpdateTime;

            // Update package catalog
            // Sleep for 30 seconds to make sure the last updated time is different after the refresh.
            Thread.Sleep(TimeSpan.FromSeconds(30));

            var updateResult = await packageCatalog.RefreshPackageCatalogAsync();
            Assert.That(updateResult, Is.Not.Null);
            Assert.That(updateResult.Status, Is.EqualTo(RefreshPackageCatalogStatus.Ok));

            packageCatalog = packageManager.GetPackageCatalogByName(options.Name);
            Assert.That(packageCatalog.Info.LastUpdateTime > lastUpdatedTime, Is.True);

            // Remove package catalog
            await this.RemovePackageCatalog();
        }

        private async Task RemovePackageCatalog()
        {
            PackageManager packageManager = this.TestFactory.CreatePackageManager();

            // Remove the tests source if it exists.
            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;

            var removeCatalogResult = await packageManager.RemovePackageCatalogAsync(removePackageCatalogOptions);
            Assert.That(removeCatalogResult, Is.Not.Null);
            Assert.That(removeCatalogResult.Status, Is.EqualTo(RemovePackageCatalogStatus.Ok));

            var packageCatalog = packageManager.GetPackageCatalogByName(removePackageCatalogOptions.Name);
            Assert.That(packageCatalog, Is.Null);
        }
    }
}
