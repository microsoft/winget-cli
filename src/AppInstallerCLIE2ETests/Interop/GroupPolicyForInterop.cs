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

            GroupPolicyException groupPolicyException = Assert.Catch<GroupPolicyException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { FindPackagesOptions findPackagesOptions = this.TestFactory.CreateFindPackagesOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = this.TestFactory.CreateCreateCompositePackageCatalogOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { InstallOptions installOptions = this.TestFactory.CreateInstallOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { UninstallOptions uninstallOptions = this.TestFactory.CreateUninstallOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { DownloadOptions downloadOptions = this.TestFactory.CreateDownloadOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { PackageMatchFilter packageMatchFilter = this.TestFactory.CreatePackageMatchFilter(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { RepairOptions repairOptions = this.TestFactory.CreateRepairOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { AddPackageCatalogOptions packageManagerSettings = this.TestFactory.CreateAddPackageCatalogOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            groupPolicyException = Assert.Catch<GroupPolicyException>(() => { RemovePackageCatalogOptions packageManagerSettings = this.TestFactory.CreateRemovePackageCatalogOptions(); });
            Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);

            // PackageManagerSettings is not implemented in context OutOfProcDev
            if (this.TestFactory.Context == ClsidContext.InProc)
            {
                groupPolicyException = Assert.Catch<GroupPolicyException>(() => { PackageManagerSettings packageManagerSettings = this.TestFactory.CreatePackageManagerSettings(); });
                Assert.AreEqual(Constants.BlockByWinGetPolicyErrorMessage, groupPolicyException.Message);
                Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, groupPolicyException.HResult);
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
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
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
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should detect the installed version
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.ModifyRepairInstaller);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Repair
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;
            var repairResult = await packageManager.RepairPackageAsync(searchResult.CatalogPackage, repairOptions);
            Assert.AreEqual(RepairResultStatus.Ok, repairResult.Status);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            Assert.True(TestCommon.VerifyTestExeUninstalled(installDir));

            // Clean up.
            if (Directory.Exists(installDir))
            {
                Directory.Delete(installDir, true);
            }

            // Download
            var downloadResult = await packageManager.DownloadPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateDownloadOptions());
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
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
            Assert.IsNotNull(addCatalogResult);
            Assert.AreEqual(AddPackageCatalogStatus.Ok, addCatalogResult.Status);

            // Get package catalog
            var packageCatalog = packageManager.GetPackageCatalogByName(options.Name);

            Assert.IsNotNull(packageCatalog);
            Assert.AreEqual(options.Name, packageCatalog.Info.Name);
            Assert.AreEqual(options.SourceUri, packageCatalog.Info.Argument);
            var lastUpdatedTime = packageCatalog.Info.LastUpdateTime;

            // Update package catalog
            // Sleep for 30 seconds to make sure the last updated time is different after the refresh.
            Thread.Sleep(TimeSpan.FromSeconds(30));

            var updateResult = await packageCatalog.RefreshPackageCatalogAsync();
            Assert.IsNotNull(updateResult);
            Assert.AreEqual(RefreshPackageCatalogStatus.Ok, updateResult.Status);

            packageCatalog = packageManager.GetPackageCatalogByName(options.Name);
            Assert.IsTrue(packageCatalog.Info.LastUpdateTime > lastUpdatedTime);

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
            Assert.IsNotNull(removeCatalogResult);
            Assert.AreEqual(RemovePackageCatalogStatus.Ok, removeCatalogResult.Status);

            var packageCatalog = packageManager.GetPackageCatalogByName(removePackageCatalogOptions.Name);
            Assert.IsNull(packageCatalog);
        }
    }
}
