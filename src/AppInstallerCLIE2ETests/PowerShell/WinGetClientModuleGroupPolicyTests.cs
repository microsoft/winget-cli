// -----------------------------------------------------------------------------
// <copyright file="WinGetClientModuleGroupPolicyTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.PowerShell
{
    using System;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Basic E2E smoke tests for verifying the Group Policy behavior for the PowerShell Microsoft.WinGet.Client module cmdlets.
    /// Running the x86 PowerShell Module requires PowerShell Core (x86). These tests currently only target PowerShell Core (x64)
    /// in the CI/CD pipeline.
    /// </summary>
    [Category("PowerShell")]
    public class WinGetClientModuleGroupPolicyTests
    {
        /// <summary>
        /// Set setup.
        /// </summary>
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source add", $"-n {Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", $"{Constants.TestSourceName}");
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void TestMethodSetup()
        {
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TestMethodTearDown()
        {
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Validates WinGet Policy Disabled.
        /// </summary>
        [Test]
        public void DisableWinGetPolicy()
        {
            // Skip x86 test run as powershell modules for x86 doesn't work as expected.
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            GroupPolicyHelper.EnableWinget.Disable();

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} exited with success status: {result.StdOut}, {result.StdErr}");
            Assert.IsNotNull(result.StdErr);
            Assert.IsTrue(result.StdErr.Contains("This operation is disabled by Group Policy : Enable Windows Package Manager"));
        }

        /// <summary>
        /// Validates EnableWinGetCLIInterfaces Policy Disabled.
        /// </summary>
        [Test]
        public void EnableWinGetCLIInterfacesPolicy()
        {
            GroupPolicyHelper.EnableWinget.Enable();
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.Disable();

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} exited with success status: {result.StdOut}, {result.StdErr}");
            Assert.IsNotNull(result.StdErr);
            Assert.IsTrue(result.StdErr.Contains("This operation is disabled by Group Policy : Enable Windows Package Manager command line interfaces"));
        }
    }
}
