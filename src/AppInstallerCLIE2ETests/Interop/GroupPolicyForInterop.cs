// -----------------------------------------------------------------------------
// <copyright file="GroupPolicyForInterop.cs" company="Microsoft Corporation">
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
    using Microsoft.WinGet.SharedLib.Exceptions;
    using NUnit.Framework;

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

            // Go through the entire install, upgrade, and uninstall flow to verify that COM calls are still supported.
            PackageManager packageManager = this.TestFactory.CreatePackageManager();
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string packageId = Constants.PortableExePackageId;
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Create composite package catalog source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            PackageCatalogReference compositeSource = packageManager.CreateCompositePackageCatalog(options);

            // Find package
            var searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure install options
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageVersionId = TestCommon.First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "1.0.0.0");

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package and verify older version was installed.
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.AreEqual(searchResult.CatalogPackage.InstalledVersion?.Version, "1.0.0.0");

            // Configure upgrade options
            var upgradeOptions = this.TestFactory.CreateInstallOptions();
            upgradeOptions.PackageVersionId = TestCommon.First(searchResult.CatalogPackage.AvailableVersions, i => i.Version == "2.0.0.0");

            // Upgrade
            var upgradeResult = await packageManager.UpgradePackageAsync(searchResult.CatalogPackage, upgradeOptions);
            Assert.AreEqual(InstallResultStatus.Ok, upgradeResult.Status);

            // Uninstall
            var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, this.TestFactory.CreateUninstallOptions());
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
        }
    }
}
