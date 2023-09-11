// -----------------------------------------------------------------------------
// <copyright file="GroupPolicyForInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
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
    }
}
