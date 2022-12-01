// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.PowerShell
{
    using NUnit.Framework;
    using System;
    using System.Diagnostics;
    using System.Linq;

    /// <summary>
    /// Basic E2E smoke tests for verifying the behavior of the PowerShell module cmdlets.
    /// Running the x86 PowerShell Module requires PowerShell Core (x86). These tests currently only target PowerShell Core (x64)
    /// </summary>
    [Category("PowerShell")]
    public class PowerShellModule
    {
        // TODO: Consider using Pester framework for conducting more extensive PowerShell module tests.

        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source add", $"-n {Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            // TODO: This is a workaround to an issue where the server takes longer than expected to terminate when
            // running from the E2E tests. This can cause other E2E tests to fail when attempting to reset the test source.
            if (IsRunning(Constants.WindowsPackageManagerServer))
            {
                // There should only be one WinGetServer process running at a time.
                Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();
                serverProcess.Kill();
            }

            TestCommon.RunAICLICommand("source remove", $"{Constants.TestSourceName}");
        }

        [Test]
        public void AssertServerShutdownAfterExecution()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}, {result.StdErr}");

            Assert.IsTrue(IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
            Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();

            // Wait a maximum of 30 seconds for the server process to exit.
            bool serverProcessExit = serverProcess.WaitForExit(30000);
            Assert.IsTrue(serverProcessExit, $"{Constants.WindowsPackageManagerServer} failed to terminate after creating COM object.");
        }

        [Test]
        public void GetWinGetSource()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var getSourceResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getSourceResult.ExitCode, $"ExitCode: {getSourceResult.ExitCode} Failed with the following output: {getSourceResult.StdOut}, {getSourceResult.StdErr}");
            Assert.IsTrue(getSourceResult.StdOut.Contains($"{Constants.TestSourceName}"));
        }

        [Test]
        public void FindWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}; {result.StdErr}");
            Assert.IsTrue(result.StdOut.Contains("TestExeInstaller"));
        }

        [Test]
        public void GetWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getResult.ExitCode, $"Failed with the following output: {getResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("TestMsiInstaller"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        [Test]
        public void InstallWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        [Test]
        public void UpdateWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            var updateResult = TestCommon.RunPowerShellCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

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
        // [Test]
        public void VerifyServerTermination()
        {
            TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            TestCommon.RunPowerShellCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            TestCommon.RunPowerShellCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.IsTrue(IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
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