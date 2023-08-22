// -----------------------------------------------------------------------------
// <copyright file="GroupPolicyForInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
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

            if (this.TestFactory.Context != ClsidContext.InProc)
            {
                COMException comException = Assert.Catch<COMException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { FindPackagesOptions findPackagesOptions = this.TestFactory.CreateFindPackagesOptions(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = this.TestFactory.CreateCreateCompositePackageCatalogOptions(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { InstallOptions installOptions = this.TestFactory.CreateInstallOptions(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { UninstallOptions uninstallOptions = this.TestFactory.CreateUninstallOptions(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { DownloadOptions downloadOptions = this.TestFactory.CreateDownloadOptions(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);

                comException = Assert.Catch<COMException>(() => { PackageMatchFilter packageMatchFilter = this.TestFactory.CreatePackageMatchFilter(); });
                Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);
            }
            else
            {
                // [NOTE:] Currently there is a design limitation in CS WinRT/C++ WinRT that the application specific error code returned from the
                // DllActivationFactory implementation will not be surfaced to caller as it is instead WinRT auto generated implementation override the actual HRESULT with one of
                // the standard HRESULT that we can expect from RoActivationInstance:
                // https://learn.microsoft.com/en-us/windows/win32/api/roapi/nf-roapi-roactivateinstance?source=recommendations  call.
                // For more details look at WinRT.cs  BaseActivationFactory::BaseActivationFactory(string typeNamespace, string typeFullName) auto generated code implementation
                // where there is a set preference that keeps HRESULT from RoGetActivationFactory over LoadLibrary call (used as a fallback approach load module form LoadLibrary).
                Assert.Throws<TargetInvocationException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });

                Assert.Throws<TargetInvocationException>(() => { FindPackagesOptions findPackagesOptions = this.TestFactory.CreateFindPackagesOptions(); });

                Assert.Throws<TargetInvocationException>(() => { CreateCompositePackageCatalogOptions createCompositePackageCatalogOptions = this.TestFactory.CreateCreateCompositePackageCatalogOptions(); });

                Assert.Throws<TargetInvocationException>(() => { InstallOptions installOptions = this.TestFactory.CreateInstallOptions(); });

                Assert.Throws<TargetInvocationException>(() => { UninstallOptions uninstallOptions = this.TestFactory.CreateUninstallOptions(); });

                Assert.Throws<TargetInvocationException>(() => { DownloadOptions downloadOptions = this.TestFactory.CreateDownloadOptions(); });

                Assert.Throws<TargetInvocationException>(() => { PackageMatchFilter packageMatchFilter = this.TestFactory.CreatePackageMatchFilter(); });

                Assert.Throws<TargetInvocationException>(() => { PackageManagerSettings packageManagerSettings = this.TestFactory.CreatePackageManagerSettings(); });
            }
        }
    }
}
