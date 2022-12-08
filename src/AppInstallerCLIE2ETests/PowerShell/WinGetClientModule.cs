// -----------------------------------------------------------------------------
// <copyright file="WinGetClientModule.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.PowerShell
{
    using System;
    using System.Diagnostics;
    using System.Linq;
    using NUnit.Framework;

    /// <summary>
    /// Basic E2E smoke tests for verifying the behavior of the PowerShell Microsoft.WinGet.Client module cmdlets.
    /// Running the x86 PowerShell Module requires PowerShell Core (x86). These tests currently only target PowerShell Core (x64).
    /// </summary>
    [Category("PowerShell")]
    public class WinGetClientModule
    {
        // TODO: Consider using Pester framework for conducting more extensive PowerShell module tests.

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
            // TODO: This is a workaround to an issue where the server takes longer than expected to terminate when
            // running from the E2E tests. This can cause other E2E tests to fail when attempting to reset the test source.
            if (this.IsRunning(Constants.WindowsPackageManagerServer))
            {
                // There should only be one WinGetServer process running at a time.
                Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();
                serverProcess.Kill();
            }

            TestCommon.RunAICLICommand("source remove", $"{Constants.TestSourceName}");
        }

        /// <summary>
        /// Assert server shutdown.
        /// </summary>
        [Test]
        public void AssertServerShutdownAfterExecution()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}, {result.StdErr}");

            Assert.IsTrue(this.IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
            Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();

            // Wait a maximum of 30 seconds for the server process to exit.
            bool serverProcessExit = serverProcess.WaitForExit(30000);
            Assert.IsTrue(serverProcessExit, $"{Constants.WindowsPackageManagerServer} failed to terminate after creating COM object.");
        }

        /// <summary>
        /// Test Get-WinGetSource.
        /// </summary>
        [Test]
        public void GetWinGetSource()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var getSourceResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getSourceResult.ExitCode, $"ExitCode: {getSourceResult.ExitCode} Failed with the following output: {getSourceResult.StdOut}, {getSourceResult.StdErr}");
            Assert.IsTrue(getSourceResult.StdOut.Contains($"{Constants.TestSourceName}"));
        }

        /// <summary>
        /// Find-WinGetPackage.
        /// </summary>
        [Test]
        public void FindWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}; {result.StdErr}");
            Assert.IsTrue(result.StdOut.Contains("TestExeInstaller"));
        }

        /// <summary>
        /// Tests Get-WinGetPackage.
        /// </summary>
        [Test]
        public void GetWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getResult.ExitCode, $"Failed with the following output: {getResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("TestMsiInstaller"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// Tests Install-WinGetPackage.
        /// </summary>
        [Test]
        public void InstallWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// Tests Update-WinGetPackage.
        /// </summary>
        [Test]
        public void UpdateWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            var updateResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"Failed with the following output: {installResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, updateResult.ExitCode, $"Failed with the following output: {updateResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getResult.ExitCode, $"Failed with the following output: {getResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(updateResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("2.0.0.0"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// There is a known issue where the server takes an abnormally long time to terminate after the E2E test pwsh processes finish execution.
        /// This test verifies that the server does indeed terminate within 5 minutes after running all of the cmdlets.
        /// Commented out to reduce the overall duration of the build pipeline.
        /// </summary>
        [Test]
        [Ignore("Ignoring")]
        public void VerifyServerTermination()
        {
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.IsTrue(this.IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
            Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();

            // Wait a maximum of 5 minutes for the server process to exit.
            bool serverProcessExit = serverProcess.WaitForExit(300000);
            Assert.IsTrue(serverProcessExit, $"{Constants.WindowsPackageManagerServer} failed to terminate after creating COM object.");
        }

        private bool IsRunning(string processName)
        {
            return Process.GetProcessesByName(processName).Length > 0;
        }
    }
}
