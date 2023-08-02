// -----------------------------------------------------------------------------
// <copyright file="PackageManagerInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading.Tasks;
    using Microsoft.CodeAnalysis.CSharp.Syntax;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using Microsoft.Management.Infrastructure;
    using NUnit.Framework;
    using WinRT;

    /// <summary>
    /// Tests check installed status.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class PackageManagerInterop : BaseInterop
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageManagerInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public PackageManagerInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            InitEnvironment(this.TestFactory.Context);
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void Teardown()
        {
            InitEnvironment(this.TestFactory.Context);
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is enabled when EnabledAppInstaller Policy NotConfigured.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerEnabledAndPolicyEnableWinGetNotConfigured()
        {
            // Scenario:
            // EnableWinGetOutOfProcessCOM = Enabled
            // EnabledAppInstaller Policy = NotConfigured.
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Enable();
            GroupPolicyHelper.EnableWinget.SetNotConfigured();
            Assert.DoesNotThrow(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is enabled and EnabledAppInstaller Policy Enabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerEnabledAndPolicyEnableWinGetEnabled()
        {
            // Scenario:
            // EnableWinGetOutOfProcessCOM = Enabled
            // EnabledAppInstaller Policy = Enabled.
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Enable();
            GroupPolicyHelper.EnableWinget.Enable();
            Assert.DoesNotThrow(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is enabled and EnabledAppInstaller Policy Disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerEnabledAndPolicyEnableWinGetDisabled()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = Enabled
            // EnabledAppInstaller Policy = Disabled.
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Enable();
            GroupPolicyHelper.EnableWinget.Disable();
            Assert.DoesNotThrow(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is NotConfigured and EnabledAppInstaller Policy Disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerNotConfiguredAndPolicyEnableWinGetNotConfigured()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = NotConfigured
            // EnabledAppInstaller Policy = NotConfigured.
            GroupPolicyHelper.EnableWinget.SetNotConfigured();
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.SetNotConfigured();
            Assert.DoesNotThrow(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is enabled and EnabledAppInstaller Policy Disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerNotConfiguredAndPolicyEnableWinGetEnabled()
        {
            // Scenario:
            // EnableWinGetOutOfProcessCOM = NotConfigured
            // EnabledAppInstaller Policy = Enabled.
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.SetNotConfigured();
            GroupPolicyHelper.EnableWinget.Enable();
            Assert.DoesNotThrow(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is enabled and EnabledAppInstaller Policy Disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerNotConfiguredAndPolicyEnableWinGetDisabled()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = NotConfigured
            // EnabledAppInstaller Policy = Disabled.
            // Expect COMException: APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY - 0x8A15003A
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.SetNotConfigured();
            GroupPolicyHelper.EnableWinget.Disable();
            COMException comException = Assert.Catch<COMException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
            Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerDisabledAndPolicyEnableWinGetNotConfigured()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = Disabled
            // EnabledAppInstaller Policy = NotConfigured
            // Expect COMException: APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY - 0x8A15003A
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Disable();
            GroupPolicyHelper.EnableWinget.SetNotConfigured();
            COMException comException = Assert.Catch<COMException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
            Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerDisabledAndPolicyEnableWinGetEnabled()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = Disabled
            // EnabledAppInstaller Policy = Enabled.
            // Expect COMException: APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY - 0x8A15003A
            GroupPolicyHelper.EnableWinget.Enable();
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Disable();
            COMException comException = Assert.Catch<COMException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
            Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);
        }

        /// <summary>
        /// Create PackageManager Instance when WinGetPackageManager policy is disabled.
        /// </summary>
        [Test]
        public void PolicyWinGetPackageManagerDisabledAndPolicyEnableWinGetDisabled()
        {
            // Scenario :
            // EnableWinGetOutOfProcessCOM = Disabled
            // EnabledAppInstaller Policy = Disabled.
            // Expect COMException: APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY - 0x8A15003A
            GroupPolicyHelper.EnableWinget.Disable();
            GroupPolicyHelper.EnableWingetPackageManagerOutOfProcessCOM.Disable();
            COMException comException = Assert.Catch<COMException>(() => { PackageManager packageManager = this.TestFactory.CreatePackageManager(); });
            Assert.AreEqual(comException.HResult, Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY);
        }

        private static void InitEnvironment(ClsidContext clsidContext)
        {
            GroupPolicyHelper.DeleteExistingPolicies();

            // To ensure Group Policy setting that tests apply correctly to WindowsPackageManagerServer (COM Server)
            // we need to be running a fresh instance of COM Server after applying Group Policy this is due to the
            // fact that the COM Server will only read Group Policy setting at the start of the process and it caches
            // it until Server Process terminates.
            if (clsidContext != ClsidContext.InProc)
            {
                try
                {
                    foreach (var process in Process.GetProcessesByName(Constants.WindowsPackageManagerServer))
                    {
                        process.Kill();
                        process.WaitForExit(30 * 1000);
                    }
                }
                catch (Exception)
                {
                    // Do nothing.
                }
            }
        }
    }
}
